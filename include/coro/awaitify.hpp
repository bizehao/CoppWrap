#pragma once

#include <memory>
#include <type_traits>
#include <async++.h>
#include "coro_wrap.hpp"

namespace cw
{

    template<typename T, typename... Args>
    void sync_wait(CoroutineContextPtr<T> coro, Args&&... args)
    {
        async::event_task<void> eventTask;
        auto task = eventTask.get_task();
        coro->setLastCallback([&eventTask]() {
            eventTask.set();
        });
        coro->start(std::forward<Args>(args)...);
        task.wait();
    }

    template<typename T, typename... Args>
    void sync_wait(CoroutineContextWrap<T> coro, Args&&... args)
    {
        sync_wait(coro.getCoroPtr(), std::forward<Args>(args)...);
    }

    template<typename T>
    T await(async::task<T>& task)
    {
        T value{};
        CoroutineContextBase* currentCoro = thisCoro();
        if (currentCoro == nullptr)
        {
            throw std::runtime_error{ "currentCoro is null" };
        }

        currentCoro->yield([currentCoro, &value]() {
            task.then([currentCoro, &value](T v) {
                value = v;
                if (currentCoro->getExecutor() != nullptr)
                {
                    currentCoro->getExecutor()->post([currentCoro]() {
                        currentCoro->resume();
                    });
                }
                else
                {
                    currentCoro->resume();
                }
            });
        });

        return value;
    }
} // namespace cw
