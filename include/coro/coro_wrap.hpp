#pragma once

#include <libcopp/coroutine/coroutine_context_container.h>
#include <memory>
#include "function_traits.hpp"

namespace cw {

class AbstractExecutor;
class CoroutineContextBase;

class BaseAbstractExecutor {
public:
    using task = std::function<void()>;
    virtual ~BaseAbstractExecutor() = default;
    virtual void post(task fun) = 0;
};

static CoroutineContextBase*& thisCoro() {
    thread_local static CoroutineContextBase* sCurCoroutine = nullptr;
    return sCurCoroutine;
};

class CoroutineContextBase {
    friend class AbstractExecutor;

public:
    ~CoroutineContextBase() {
    }
    void resume() {
        thisCoro() = this;
        _coObj->resume();
        if (_coObj->check_flags(coroutine_type::status_type::EN_CRS_FINISHED)) {
            _coObj = nullptr;
        }
    }
    void yield() {
        thisCoro() = nullptr;
        _coObj->yield();
    }

    void yield(std::function<void()> fun) {
        thisCoro() = nullptr;
        _coObj->yield(std::move(fun));
    }

    bool isFinished() {
        return _coObj->is_finished();
    }

    void addLastCallback(std::function<void()> callback) {
        _lastCallbacks.push_back(std::move(callback));
    }

    BaseAbstractExecutor* getExecutor() {
        return _executor;
    }

protected:
    using coroutine_type = copp::coroutine_context_default;
    coroutine_type::ptr_t _coObj;
    BaseAbstractExecutor* _executor{nullptr};
    std::vector<std::function<void()>> _lastCallbacks;

    friend void runOn(BaseAbstractExecutor& executor);
};

template <typename T>
class CoroutineContext;

template <typename... Args>
class CoroutineContext<std::tuple<Args...>> : public CoroutineContextBase,
                                              public std::enable_shared_from_this<CoroutineContext<std::tuple<Args...>>> {
    using ArgsTup = std::tuple<Args...>;

    friend class AbstractExecutor;

public:
    CoroutineContext(std::function<void(Args...)> fun) :
        _fun{fun} {
    }

    ~CoroutineContext() {
        // std::cout << "析构了" << std::endl;
    }

    void start(Args... args) {
        ArgsTup tup = std::forward_as_tuple(args...);
        startAndOp(tup);
    }

    void start(ArgsTup tup) {
        startAndOp(tup);
    }

private:
    void startAndOp(ArgsTup tup) {
        if (_coObj) {
            throw std::runtime_error{"Started multiple times"};
        }
        _coObj = coroutine_type::create([self = getSelf()](void* data) {
            // std::cout << "count1: " << self.use_count() << std::endl;
            ArgsTup* tup = reinterpret_cast<ArgsTup*>(data);
            self->invoke(self->_fun, tup, std::make_index_sequence<std::tuple_size_v<ArgsTup>>{});
            // std::cout << "count2: " << self.use_count() << std::endl;
            return 0;
        });

        if (!_lastCallbacks.empty()) {
            _coObj->set_last_callback([self = getSelf()]() {
                for (auto f : self->_lastCallbacks) {
                    f();
                }
            });
        }

        thisCoro() = this;
        _coObj->start(&tup);
        if (_coObj->check_flags(coroutine_type::status_type::EN_CRS_FINISHED)) {
            _coObj = nullptr;
        }
    }

    template <typename Fun, typename Tup, size_t... I>
    void invoke(Fun&& fun, Tup* tup, std::index_sequence<I...>) {
        fun(std::get<I>(*tup)...);
    }

    std::shared_ptr<CoroutineContext<std::tuple<Args...>>> getSelf() {
        using supper_this = std::enable_shared_from_this<CoroutineContext<std::tuple<Args...>>>;
        return supper_this::shared_from_this();
    }

private:
    std::function<void(Args...)> _fun;
};

using CoroutineContextBasePtr = std::shared_ptr<CoroutineContextBase>;

template <typename T>
using CoroutineContextPtr = std::shared_ptr<CoroutineContext<T>>;

template <typename T>
class CoroutineContextWrap;

template <typename... Args>
class CoroutineContextWrap<std::tuple<Args...>> {
public:
    CoroutineContextWrap(CoroutineContextPtr<std::tuple<Args...>> ptr) :
        coroPtr{ptr} {
    }

    void start(Args... args) {
        coroPtr->start(args...);
    }

    void start(std::tuple<Args...> tup) {
        coroPtr->start(tup);
    }

    void operator()(Args... args) {
        coroPtr->start(args...);
    }

    operator CoroutineContextPtr<std::tuple<Args...>>() {
        return coroPtr;
    }

    operator std::function<void(Args...)>() {
        auto lambda = [tmpCoroPtr = coroPtr](Args... args) {
            tmpCoroPtr->start(args...);
        };
        return lambda;
    }

    CoroutineContextPtr<std::tuple<Args...>> getCoroPtr() {
        return coroPtr;
    }

private:
    CoroutineContextPtr<std::tuple<Args...>> coroPtr;
};

template <typename Fun, typename ArgsTup = typename detail::function_traits<Fun>::arg_types>
CoroutineContextWrap<ArgsTup> createCoroutineContext(Fun&& fun) {
    static_assert(detail::is_function<Fun>::value);
    static_assert(std::is_same<typename detail::function_traits<Fun>::return_type, void>::value);
    return std::make_shared<CoroutineContext<ArgsTup>>(std::forward<Fun>(fun));
}

namespace detail {
template <typename ArgsTup, size_t... I>
void awaitImpl(std::shared_ptr<CoroutineContext<ArgsTup>> coroPtr,
               std::index_sequence<I...>,
               std::tuple_element_t<I, ArgsTup>... args) {
    CoroutineContextBase* currentCoro = thisCoro();
    if (currentCoro == nullptr) {
        throw std::runtime_error{"currentCoro is null"};
    } else {
        coroPtr->addLastCallback([currentCoro]() {
            if (currentCoro->getExecutor() != nullptr) {
                currentCoro->getExecutor()->post([currentCoro]() {
                    currentCoro->resume();
                });
            } else {
                currentCoro->resume();
            }
        });
        ArgsTup data = std::forward_as_tuple(args...);
        currentCoro->yield([=]() {
            coroPtr->start(data);
        });
    }
}
} // namespace detail

template <typename ArgsTup, typename... Args>
void await(CoroutineContextPtr<ArgsTup> coroPtr, Args&&... args) {
    detail::awaitImpl(coroPtr, std::make_index_sequence<std::tuple_size_v<ArgsTup>>{}, std::forward<Args>(args)...);
}

template <typename ArgsTup, typename... Args>
void await(CoroutineContextWrap<ArgsTup> coroWrap, Args&&... args) {
    detail::awaitImpl(coroWrap.getCoroPtr(), std::make_index_sequence<std::tuple_size_v<ArgsTup>>{}, std::forward<Args>(args)...);
}

void runOn(BaseAbstractExecutor& executor) {
    CoroutineContextBase* coro = thisCoro();
    coro->_executor = &executor;
    coro->yield([coro, &executor]() {
        executor.post([coro]() {
            coro->resume();
        });
    });
}

#define Coro cw::CoroMacroImpl{} +

struct CoroMacroImpl {
private:
    template <typename ArgsTup, typename Fun, size_t... I>
    std::function<void(std::tuple_element_t<I, ArgsTup>...)> makeFun(Fun&& fun, std::index_sequence<I...>) {
        auto coroFun = createCoroutineContext(std::forward<Fun>(fun));
        return coroFun;
    }

public:
    template <typename Fun>
    auto operator+(Fun&& fun) {
        using arg_types = typename detail::function_traits<Fun>::arg_types;
        return makeFun<arg_types>(std::forward<Fun>(fun), std::make_index_sequence<std::tuple_size_v<arg_types>>{});
    }
};

} // namespace cw