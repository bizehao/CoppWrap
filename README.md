# CoppWrap
A packaging with stack protocol, based on [libcopp](https://github.com/owent/libcopp)

```
cw::ManualExecutor manualExecutor;
cw::ThreadPool threadPool;
cw::ThreadPool threadPoolBB;

template<typename Obj, typename MemFun, typename Fun>
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
    std::this_thread::sleep_for(std::chrono::seconds{ sec });
}

int main() {
    system("chcp 65001");

    auto timeOut = cw::createCoroutineContext([](int sec) {
        await(cw::createCoroutineContext(taskFun), 3);
        std::cout << "5 taskFun执行完成: " << std::this_thread::get_id() << std::endl;
        runOn(threadPool);
        std::cout << "6 timeOut中执行耗时任务: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds{ sec });
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

    while (true) {
        manualExecutor.wait_for_task();
        manualExecutor.loop_once();
    }

    return 0;
}
```
