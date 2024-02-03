#pragma once

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <utility>
#include <vector>
#include "executor.hpp"

namespace cw {

    template <typename T, typename = typename std::enable_if<std::is_move_assignable<T>::value>::type>
    class Queue {
    public:
        void push(T&& item) {
            {
                std::lock_guard<std::mutex> guard{ _mutex };
                _queue.push(std::move(item));
            }
            _cond.notify_one();
        }

        bool try_push(const T& item) {
            {
                std::unique_lock<std::mutex> lock{ _mutex, std::try_to_lock };
                if (!lock) return false;
                _queue.push(item);
            }
            _cond.notify_one();
            return true;
        }

        bool pop(T& item) {
            std::unique_lock<std::mutex> lock{ _mutex };
            _cond.wait(lock, [&]() { return !_queue.empty() || _stop; });
            if (_queue.empty()) return false;
            item = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        bool try_pop(T& item) {
            std::unique_lock<std::mutex> lock{ _mutex, std::try_to_lock };
            if (!lock || _queue.empty()) return false;

            item = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        // non-blocking pop an item, maybe pop failed.
        // predict is an extension pop condition, default is null.
        bool try_pop_if(T& item, bool (*predict)(T&) = nullptr) {
            std::unique_lock<std::mutex> lock{ _mutex, std::try_to_lock };
            if (!lock || _queue.empty()) return false;

            if (predict && !predict(_queue.front())) {
                return false;
            }

            item = std::move(_queue.front());
            _queue.pop();
            return true;
        }

        std::size_t size() const {
            std::lock_guard<std::mutex> guard{ _mutex };
            return _queue.size();
        }

        bool empty() const {
            std::lock_guard<std::mutex> guard{ _mutex };
            return _queue.empty();
        }

        void stop() {
            {
                std::lock_guard<std::mutex> guard{ _mutex };
                _stop = true;
            }
            _cond.notify_all();
        }

        void clear() {
            std::lock_guard<std::mutex> guard{ _mutex };
            _queue = std::queue<T>{};
        }

    private:
        std::queue<T> _queue;
        bool _stop = false;
        mutable std::mutex _mutex;
        std::condition_variable _cond;
    };

    class ThreadPool : public AbstractExecutor {
    public:
        struct WorkItem {
            bool canSteal{ false };
            std::function<void()> fn;
        };

        explicit ThreadPool(std::size_t threadNum = std::thread::hardware_concurrency(), bool enableWorkSteal = false)
            : _threadNum(threadNum ? threadNum : std::thread::hardware_concurrency()),
            _queues{ _threadNum },
            _stop{ false },
            _enableWorkSteal{ enableWorkSteal } {
            auto worker = [this](std::size_t id) {
                auto current = get_current();
                current->first = id;
                current->second = this;
                while (!_stop) {
                    WorkItem workerItem = {};
                    if (_enableWorkSteal) {
                        // Try to do work steal firstly.
                        for (auto n = 0; n < _threadNum * 2; ++n) {
                            if (_queues[(id + n) % _threadNum].try_pop_if(workerItem, [](auto& item) { return item.canSteal; })) break;
                        }
                    }

                    if (!workerItem.fn && !_queues[id].pop(workerItem)) continue;
                    workerItem.fn();
                    _taskNum--;
                    _cv.notify_all();
                }
                };

            _threads.reserve(_threadNum);

            for (auto i = 0; i < _threadNum; ++i) {
                _threads.emplace_back(worker, i);
            }
        }
        ~ThreadPool() override {
            _stop = true;
            for (auto& queue : _queues) {
                queue.stop();
            }

            for (auto& thread : _threads) {
                thread.join();
            }
        }

        void post(task fun) override {
            int id = -1;
            if (_enableWorkSteal) {
                // Try to push to a non-block queue firstly.
                for (auto n = 0; n < _threadNum * 2; ++n) {
                    if (_queues.at((id + n) % _threadNum).try_push(WorkItem{/*canSteal = */ _enableWorkSteal, std::move(fun) }))
                        return;
                }
            }
            id = rand() % _threadNum;
            _queues[id].push(WorkItem{/*canSteal = */ _enableWorkSteal, std::move(fun) });
        }

        template <typename Fun, typename Result = decltype(std::declval<Fun>()())>
        std::future<Result> post(Fun&& fun, int32_t id = -1) {
            if (_stop) {
                throw std::runtime_error{ "ERROR_POOL_HAS_STOP" };
            }

            _taskNum++;
            auto pt = new std::packaged_task<Result()>{ fun };
            auto result = pt->get_future();
            auto callback = [pt]() {
                (*pt)();
                delete pt;
                };

            if (id == -1) {
                if (_enableWorkSteal) {
                    // Try to push to a non-block queue firstly.
                    for (auto n = 0; n < _threadNum * 2; ++n) {
                        if (_queues.at((id + n) % _threadNum)
                            .try_push(WorkItem{/*canSteal = */ _enableWorkSteal, std::move(callback) }))
                            return result;
                    }
                }
                id = rand() % _threadNum;
                _queues[id].push(WorkItem{/*canSteal = */ _enableWorkSteal, std::move(callback) });
            }
            else {
                assert(id < _threadNum);
                _queues[id].push(WorkItem{/*canSteal = */ false, std::move(callback) });
            }
            return result;
        }

        std::size_t get_current_id() const {
            auto current = get_current();
            if (this == current->second) {
                return current->first;
            }
            return std::numeric_limits<std::size_t>::max();
        }

        std::size_t get_item_count() const {
            std::size_t ret = 0;
            for (auto i = 0; i < _threadNum; ++i) {
                ret += _queues[i].size();
            }
            return ret;
        }

        std::size_t get_thread_num() const { return _threadNum; }

        void wait() {
            std::unique_lock<std::mutex> lock{ _mutex };
            _cv.wait(lock, [this]() { return _taskNum == 0; });
        }

        template <class Clock, class Duration>
        bool wait_until(const std::chrono::time_point<Clock, Duration>& absTime) {
            std::unique_lock<std::mutex> lock{ _mutex };
            bool result = _cv.wait_until(lock, absTime, [this]() { return _taskNum == 0; });
            return result;
        }

        template <class Rep, class Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& relTime) {
            return wait_until(std::chrono::system_clock::now() + relTime);
        }

        void cancel() {
            std::lock_guard<std::mutex> guard{ _mutex };
            for (auto& it : _queues) {
                it.clear();
            }
        }

    private:
        std::pair<std::size_t, ThreadPool*>* get_current() const {
            static thread_local std::pair<std::size_t, ThreadPool*> current(-1, nullptr);
            return &current;
        }

        std::size_t _threadNum;

        std::vector<Queue<WorkItem>> _queues;
        std::vector<std::thread> _threads;

        std::atomic<bool> _stop;
        bool _enableWorkSteal;

        std::mutex _mutex;
        std::condition_variable _cv;
        std::atomic<int> _taskNum;
    };
}
