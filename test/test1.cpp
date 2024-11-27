// CoppWrap.cpp: 定义应用程序的入口点。
//

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
#include <format>
#include <noboost/pool/pool.hpp>
#include <chrono>
#include <coro/manual_executor.hpp>
#include <functional>
#include <coro/move_wrapper.hpp>


namespace ctx = boost::context;

ctx::fiber resume_on(ctx::fiber&& f, cw::AbstractExecutor& executor)
{
    return std::move(f).resume_with([&](ctx::fiber&& fc) {
        executor.post([fc = folly::makeMoveWrapper(std::move(fc))]() mutable {
            auto pp = fc.move().resume();
        });
        return ctx::fiber{};
    });
}

int main()
{

    system("chcp 65001");

    static ctx::fiber ffo;

    ffo = ctx::fiber{ [&](ctx::fiber&& f2) {
        std::cout << "111" << std::endl;
        ffo = std::move(f2).resume(); //yield
        std::cout << "333" << std::endl;
        return std::move(ffo);
    } };
    std::cout << "000" << std::endl;
    ffo = std::move(ffo).resume();
    std::cout << "222" << std::endl;
    ffo = std::move(ffo).resume();
    std::cout << "444" << std::endl;
    //f1();


    /*cw::ManualExecutor manualExecutor;
    cw::ThreadPool threadPool{ 1 };

    ctx::fiber f1{ [&](ctx::fiber&& f2) {
        std::cout << "thread3 id: " << std::this_thread::get_id() << std::endl;

        f2 = resume_on(std::move(f2), threadPool);

        std::cout << "thread4 id: " << std::this_thread::get_id() << std::endl;

        f2 = resume_on(std::move(f2), manualExecutor);

        std::cout << "thread5 id: " << std::this_thread::get_id() << std::endl;

        return std::move(f2);
    } };

    manualExecutor.post([fo1 = folly::makeMoveWrapper(std::move(f1))]() mutable {
        std::cout << "thread1 id: " << std::this_thread::get_id() << std::endl;
        auto ff = fo1.move().resume();
        std::cout << "thread2 id: " << std::this_thread::get_id() << std::endl;
    });

    while (true)
    {
        manualExecutor.wait_for_task();
        manualExecutor.loop_once();
    }*/

    return 0;
}
