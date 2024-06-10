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

#include <boost/context/fiber.hpp>
#include <boost/context/continuation.hpp>
#include <boost/asio.hpp>

#include <chrono>
#include <future>

#include <format>

cw::ManualExecutor manualExecutor;
cw::ThreadPool threadPool;
cw::ThreadPool threadPoolBB;

//
template <typename Obj, typename MemFun, typename Fun>
void connect(Obj* obj, MemFun memFUn, Fun&& fun) {
    obj->_fun = fun;
}

class Button {
public:
    void click(bool checked) {
        _fun(checked);
    }

    std::function<void(bool)> _fun;
};

void taskFun(int sec) {
    runOn(threadPool);
    std::cout << "4 taskFun中执行耗时任务: " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds{sec});
}

using namespace std::chrono_literals;

//#define My_Test

int main() {
    system("chcp 65001");

    /*namespace ctx = boost::context;
    int data = 0;
    ctx::fiber f1{[&data](ctx::fiber&& f2) {
        std::cout << "f1: entered first time: " << data << std::endl;
        data += 1;
        f2 = std::move(f2).resume();
        std::cout << "f1: entered second time: " << data << std::endl;
        data += 1;
        f2 = std::move(f2).resume();
        std::cout << "f1: entered third time: " << data << std::endl;
        return std::move(f2);
    }};
    f1 = std::move(f1).resume();
    std::cout << "f1: returned first time: " << data << std::endl;
    data += 1;
    f1 = std::move(f1).resume();
    std::cout << "f1: returned second time: " << data << std::endl;
    data += 1;
    f1 = std::move(f1).resume_with([&data](ctx::fiber&& f2) {
        std::cout << "f2: entered: " << data << std::endl;
        data = -1;
        return std::move(f2);
    });
    std::cout << "f1: returned third time" << std::endl;*/

    namespace ctx = boost::context;
    namespace asio = boost::asio;
    asio::io_context io_context;
    asio::thread_pool thread_pool;

    auto work = asio::make_work_guard(io_context);

    {
        auto co1 = cw::createCoroutineContext([](int a, double b) {
            std::cout << std::format("a: {}, b: {}", a, b) << std::endl;
            cw::runOn(threadPool);
            std::cout << "start exec task" << std::endl;
            std::this_thread::sleep_for(3s);
            std::cout << "task finish" << std::endl;
            cw::runOn(threadPoolBB);
            std::cout << "exit" << std::endl;
        });
        co1.start(100, 3.14);
    }
    

    io_context.run();

#ifdef My_Test

    auto timeOut = cw::createCoroutineContext([](int sec) {
        await(cw::createCoroutineContext(taskFun), 3);
        std::cout << "5 taskFun执行完成: " << std::this_thread::get_id() << std::endl;
        runOn(threadPool);
        std::cout << "6 timeOut中执行耗时任务: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds{sec});
    });

    std::cout << "0 主线程: " << std::this_thread::get_id() << std::endl;
    Button bitton;
    connect(
        &bitton,
        &Button::click,
        Coro[timeOut](bool checked) {
            std::cout << std::boolalpha;
            std::cout << "1 在主线程操作: " << std::this_thread::get_id() << " checked: " << checked << std::endl;
            checked = false;
            runOn(threadPoolBB);
            std::cout << "2 切换到子线程操作: " << std::this_thread::get_id() << " checked: " << checked << std::endl;
            checked = true;
            std::cout << "3 等待timeOut结束: " << std::this_thread::get_id() << " checked: " << checked << std::endl;
            await(timeOut, 3);
            std::cout << "7 timeOut结束了: " << std::this_thread::get_id() << " checked: " << checked << std::endl;
            runOn(manualExecutor);
            std::cout << "8 切换到主线程操作: " << std::this_thread::get_id() << " checked: " << checked << std::endl;
        });

    bitton.click(true);
#endif // My_Test

    //auto task1 = async::spawn([] {
    //    std::cout << "exec task1" << std::endl;
    //    std::this_thread::sleep_for(std::chrono::seconds{3});
    //    std::cout << "Task 1 executes asynchronously" << std::endl;
    //    return 100;
    //});

    //auto timeOut = cw::createCoroutineContext([&task1](int sec) {
    //    auto rst = cw::await(task1);
    //    std::cout << "5 taskFun执行完成: " << std::this_thread::get_id() << " rst: " << rst << std::endl;
    //    runOn(threadPool);
    //    std::cout << "6 timeOut中执行耗时任务: " << std::this_thread::get_id() << std::endl;
    //});

    //cw::sync_wait(timeOut, 2);
    //std::cout << "等待结束" << std::endl;

#ifdef My_Test
    while (true) {
        manualExecutor.wait_for_task();
        manualExecutor.loop_once();
    }
#endif

    return 0;
}
