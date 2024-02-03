#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <numeric>
#include <string>
#include "executor.hpp"

namespace cw {

    inline void throw_runtime_shutdown_exception(const std::string& executor_name) {
        const auto error_msg = std::string(executor_name);
        throw std::runtime_error{ error_msg };
    }

    class ManualExecutor final : public AbstractExecutor {
    private:
        mutable std::mutex m_lock;
        std::deque<task> m_tasks;
        std::condition_variable m_condition;
        bool m_abort{ false };
        std::atomic_bool m_atomic_abort{ false };
        inline static const std::string name{ "ManualExecutor" };

        template <class clock_type, class duration_type>
        static std::chrono::system_clock::time_point to_system_time_point(
            std::chrono::time_point<clock_type, duration_type> time_point) noexcept(noexcept(clock_type::now())) {
            const auto src_now = clock_type::now();
            const auto dst_now = std::chrono::system_clock::now();
            return dst_now + std::chrono::duration_cast<std::chrono::milliseconds>(time_point - src_now);
        }

        static std::chrono::system_clock::time_point time_point_from_now(std::chrono::milliseconds ms) noexcept {
            return std::chrono::system_clock::now() + ms;
        }

        size_t loop_impl(size_t max_count) {
            if (max_count == 0) {
                return 0;
            }

            size_t executed = 0;

            while (true) {
                if (executed == max_count) {
                    break;
                }

                std::unique_lock<decltype(m_lock)> lock(m_lock);
                if (m_abort) {
                    break;
                }

                if (m_tasks.empty()) {
                    break;
                }

                auto task = std::move(m_tasks.front());
                m_tasks.pop_front();
                lock.unlock();

                task();
                ++executed;
            }

            if (shutdown_requested()) {
                throw_runtime_shutdown_exception(name);
            }

            return executed;
        }

        size_t loop_until_impl(size_t max_count, std::chrono::time_point<std::chrono::system_clock> deadline) {
            if (max_count == 0) {
                return 0;
            }

            size_t executed = 0;
            deadline += std::chrono::milliseconds(1);

            while (true) {
                if (executed == max_count) {
                    break;
                }

                const auto now = std::chrono::system_clock::now();
                if (now >= deadline) {
                    break;
                }

                std::unique_lock<decltype(m_lock)> lock(m_lock);
                const auto found_task = m_condition.wait_until(lock, deadline, [this] { return !m_tasks.empty() || m_abort; });

                if (m_abort) {
                    break;
                }

                if (!found_task) {
                    break;
                }

                assert(!m_tasks.empty());
                auto task = std::move(m_tasks.front());
                m_tasks.pop_front();
                lock.unlock();

                task();
                ++executed;
            }

            if (shutdown_requested()) {
                throw_runtime_shutdown_exception(name);
            }

            return executed;
        }

        void wait_for_tasks_impl(size_t count) {
            if (count == 0) {
                if (shutdown_requested()) {
                    throw_runtime_shutdown_exception(name);
                }
                return;
            }

            std::unique_lock<decltype(m_lock)> lock(m_lock);
            m_condition.wait(lock, [this, count] { return (m_tasks.size() >= count) || m_abort; });

            if (m_abort) {
                throw_runtime_shutdown_exception(name);
            }

            assert(m_tasks.size() >= count);
        }

        size_t wait_for_tasks_impl(size_t count, std::chrono::time_point<std::chrono::system_clock> deadline) {
            deadline += std::chrono::milliseconds(1);

            std::unique_lock<decltype(m_lock)> lock(m_lock);
            m_condition.wait_until(lock, deadline, [this, count] { return (m_tasks.size() >= count) || m_abort; });

            if (m_abort) {
                throw_runtime_shutdown_exception(name);
            }

            return m_tasks.size();
        }

    public:
        ManualExecutor() {}

        void post(task task) override {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            if (m_abort) {
                throw_runtime_shutdown_exception(name);
            }

            m_tasks.emplace_back(std::move(task));
            lock.unlock();

            m_condition.notify_all();
        }
        // void enqueue(std::span<task> tasks);

        int max_concurrency_level() const noexcept { return std::numeric_limits<int>::max(); }

        void shutdown() {
            const auto abort = m_atomic_abort.exchange(true, std::memory_order_relaxed);
            if (abort) {
                return;  // shutdown had been called before.
            }

            decltype(m_tasks) tasks;

            {
                std::unique_lock<decltype(m_lock)> lock(m_lock);
                m_abort = true;
                tasks = std::move(m_tasks);
            }

            m_condition.notify_all();

            tasks.clear();
        }
        bool shutdown_requested() const { return m_atomic_abort.load(std::memory_order_relaxed); }

        size_t size() const {
            std::unique_lock<std::mutex> lock(m_lock);
            return m_tasks.size();
        }
        bool empty() const { return size() == 0; }

        size_t clear() {
            std::unique_lock<decltype(m_lock)> lock(m_lock);
            if (m_abort) {
                throw_runtime_shutdown_exception(name);
            }

            const auto tasks = std::move(m_tasks);
            lock.unlock();
            return tasks.size();
        }

        bool loop_once() { return loop_impl(1) != 0; }
        bool loop_once_for(std::chrono::milliseconds max_waiting_time) {
            if (max_waiting_time == std::chrono::milliseconds(0)) {
                return loop_impl(1) != 0;
            }

            return loop_until_impl(1, time_point_from_now(max_waiting_time));
        }

        template <class clock_type, class duration_type>
        bool loop_once_until(std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return loop_until_impl(1, to_system_time_point(timeout_time));
        }

        size_t loop(size_t max_count) { return loop_impl(max_count); }
        size_t loop_for(size_t max_count, std::chrono::milliseconds max_waiting_time) {
            if (max_count == 0) {
                return 0;
            }

            if (max_waiting_time == std::chrono::milliseconds(0)) {
                return loop_impl(max_count);
            }

            return loop_until_impl(max_count, time_point_from_now(max_waiting_time));
        }

        template <class clock_type, class duration_type>
        size_t loop_until(size_t max_count, std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return loop_until_impl(max_count, to_system_time_point(timeout_time));
        }

        void wait_for_task() { wait_for_tasks_impl(1); }
        bool wait_for_task_for(std::chrono::milliseconds max_waiting_time) {
            return wait_for_tasks_impl(1, time_point_from_now(max_waiting_time)) == 1;
        }

        template <class clock_type, class duration_type>
        bool wait_for_task_until(std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return wait_for_tasks_impl(1, to_system_time_point(timeout_time)) == 1;
        }

        void wait_for_tasks(size_t count) { wait_for_tasks_impl(count); }
        size_t wait_for_tasks_for(size_t count, std::chrono::milliseconds max_waiting_time) {
            return wait_for_tasks_impl(count, time_point_from_now(max_waiting_time));
        }

        template <class clock_type, class duration_type>
        size_t wait_for_tasks_until(size_t count, std::chrono::time_point<clock_type, duration_type> timeout_time) {
            return wait_for_tasks_impl(count, to_system_time_point(timeout_time));
        }
    };

}