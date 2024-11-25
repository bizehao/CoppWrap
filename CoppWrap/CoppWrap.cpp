// CoppWrap.cpp: 定义应用程序的入口点。
//
#include "CoppWrap.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <coro/manual_executor.hpp>
#include <coro/thread_pool.hpp>
#include <coro/coro_wrap.hpp>
#include <coro/awaitify.hpp>
#include <noboost/context/fiber.hpp>
#include <chrono>
#include <future>
#include <chrono>
#include <coro/manual_executor.hpp>
#include <functional>
#include <coro/move_wrapper.hpp>
#include <sstream>
#include <mutex>

cw::ManualExecutor manualExecutor;
cw::ThreadPool threadPoolA;
cw::ThreadPool threadPoolB;
cw::ThreadPool threadPoolC;

struct sync_count : std::stringstream
{
    ~sync_count()
    {
        static std::mutex _mutex;
        std::lock_guard guard{ _mutex };
        std::cout << this->rdbuf();
    }
};

template<typename Obj, typename MemFun, typename Fun>
void connect(Obj* obj, MemFun memFUn, Fun&& fun)
{
    obj->_fun = fun;
}

class Button
{
public:
    void click(bool checked)
    {
        _fun(checked);
    }

    std::function<void(bool)> _fun;
};

using namespace std::chrono_literals;
namespace ctx = boost::context;

int main()
{
    system("chcp 65001");

    {
        sync_count{} << "主线程: " << std::this_thread::get_id() << std::endl;
        for (int i = 0; i < 1; i++)
        {
            auto timeOut = cw::createCoroutineContext([](int sec) {
                std::this_thread::sleep_for(std::chrono::seconds{ sec });
            });

            auto asyncHandle = cw::createCoroutineContext([](std::string arg) {
                runOn(threadPoolC);
                sync_count{} << "开始处理异步任务: " << std::this_thread::get_id() << std::endl;
                sync_count{} << "参数: " << arg << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds{ 3 });
                sync_count{} << "结束处理异步任务: " << std::this_thread::get_id() << std::endl;
            });

            Button bitton;
            connect(
                &bitton,
                &Button::click,
                Coro[timeOut, asyncHandle](bool checked) {
                    sync_count{} << "当前在主线程: " << std::this_thread::get_id() << std::endl;
                    runOn(threadPoolA);
                    sync_count{} << "切换到子线程A: " << std::this_thread::get_id() << std::endl;
                    runOn(threadPoolB);
                    sync_count{} << "切换到子线程B: " << std::this_thread::get_id() << std::endl;
                    await(timeOut, 3);
                    sync_count{} << "等待了3s, 继续在线程B: " << std::this_thread::get_id() << std::endl;
                    runOn(manualExecutor);
                    sync_count{} << "当前在主线程：" << std::this_thread::get_id() << std::endl;
                    await(asyncHandle, std::string{ "一些参数" });
                    sync_count{} << "返回到主线程：" << std::this_thread::get_id() << std::endl;
                });

            bitton.click(true);
        }
        
    }

    //验证主线程不阻塞
    std::thread main_out{
        []() {
            while (true)
            {
                manualExecutor.post([]() {
                    //sync_count{} << "--" << std::endl;
                });
                std::this_thread::sleep_for(100ms);
            }
        }
    };

    while (true)
    {
        manualExecutor.wait_for_task();
        manualExecutor.loop_once();
    }

    main_out.join();

    return 0;
}
