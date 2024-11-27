#pragma once

#include <memory>
#include "function_traits.hpp"
#include <noboost/context/fiber.hpp>
#include "move_wrapper.hpp"

namespace cw
{

    namespace ctx = boost::context;

    class AbstractExecutor;
    class CoroutineContextBase;

    class BaseAbstractExecutor
    {
    public:
        using task = std::function<void()>;
        virtual ~BaseAbstractExecutor() = default;
        virtual void post(task fun) = 0;
    };

    static CoroutineContextBase*& thisCoro()
    {
        thread_local CoroutineContextBase* sCurCoroutine = nullptr;
        return sCurCoroutine;
    };

    class CoroutineContextBase
    {
        friend class AbstractExecutor;

    public:
        CoroutineContextBase()
        {
        }
        ~CoroutineContextBase()
        {
        }
        void resume()
        {
            thisCoro() = this;
            _source = std::move(_source).resume();
            if (_yield_after_call)
            {
                std::function<void()> exec = std::move(_yield_after_call);
                exec();
            }
            finish_callback();
        }

        // 恢复到切入点的时侯 call fun
        void yield(std::function<void()> fun = nullptr)
        {
            thisCoro() = nullptr;
            _yield_after_call = std::move(fun);
            _source = std::move(_source).resume();
        }

        bool isFinished()
        {
            return _finished;
        }

        void setLastCallback(std::function<void()> callback)
        {
            _last_call = std::move(callback);
        }

        BaseAbstractExecutor* getExecutor()
        {
            return _executor;
        }

        void destory()
        {
            _source = {};
            finish_callback();
        }

    protected:
        void finish_callback()
        {
            if (isFinished())
            {
                if (_last_call)
                {
                    _last_call();
                }
                _prolong_life.reset();
            }
        }

        ctx::fiber _source;
        std::function<void()> _yield_after_call;
        BaseAbstractExecutor* _executor{ nullptr };
        std::function<void()> _last_call;
        bool _finished{ false };
        std::shared_ptr<CoroutineContextBase> _prolong_life;
        friend void runOn(BaseAbstractExecutor& executor);
    };

    template<typename T>
    class CoroutineContext;

    template<typename... Args>
    class CoroutineContext<std::tuple<Args...>> : public CoroutineContextBase, public std::enable_shared_from_this<CoroutineContext<std::tuple<Args...>>>
    {
        using ArgsTup = std::tuple<Args...>;

        friend class AbstractExecutor;

    public:
        CoroutineContext(std::function<void(Args...)> fun) :
            _fun{ fun }
        {
        }

        ~CoroutineContext()
        {
        }

        void start(Args... args)
        {
            ArgsTup tup = std::forward_as_tuple(args...);
            startAndOp(tup);
        }

        void start(ArgsTup tup)
        {
            startAndOp(tup);
        }

    private:
        void startAndOp(ArgsTup tup)
        {
            if (_source)
            {
                throw std::runtime_error{ "Started multiple times" };
            }
            _source = ctx::fiber{ [self = getSelf(), &tup](ctx::fiber&& fiber) {
                self->_source = std::move(fiber);
                self->_prolong_life = self;
                self->invoke(self->_fun, tup, std::make_index_sequence<std::tuple_size_v<ArgsTup>>{});
                self->_finished = true;
                return std::move(self->_source);
            } };
            resume();
        }

        template<typename Fun, typename Tup, size_t... I>
        void invoke(Fun&& fun, Tup&& tup, std::index_sequence<I...>)
        {
            fun(std::get<I>(std::forward<Tup>(tup))...);
        }

        std::shared_ptr<CoroutineContext<std::tuple<Args...>>> getSelf()
        {
            using supper_this = std::enable_shared_from_this<CoroutineContext<std::tuple<Args...>>>;
            return supper_this::shared_from_this();
        }

    private:
        std::function<void(Args...)> _fun;
    };

    using CoroutineContextBasePtr = std::shared_ptr<CoroutineContextBase>;

    template<typename T>
    using CoroutineContextPtr = std::shared_ptr<CoroutineContext<T>>;

    template<typename T>
    class CoroutineContextWrap;

    template<typename... Args>
    class CoroutineContextWrap<std::tuple<Args...>>
    {
    public:
        CoroutineContextWrap(CoroutineContextPtr<std::tuple<Args...>> ptr) :
            coroPtr{ ptr }
        {
        }

        void start(Args... args)
        {
            coroPtr->start(args...);
        }

        void start(std::tuple<Args...> tup)
        {
            coroPtr->start(tup);
        }

        void resume()
        {
            coroPtr->resume();
        }

        void operator()(Args... args)
        {
            coroPtr->start(args...);
        }

        operator CoroutineContextPtr<std::tuple<Args...>>()
        {
            return coroPtr;
        }

        operator std::function<void(Args...)>()
        {
            auto lambda = [tmpCoroPtr = coroPtr](Args... args) {
                tmpCoroPtr->start(args...);
            };
            return lambda;
        }

        CoroutineContextPtr<std::tuple<Args...>> getCoroPtr()
        {
            return coroPtr;
        }

    private:
        CoroutineContextPtr<std::tuple<Args...>> coroPtr;
    };

    template<typename Fun, typename ArgsTup = typename detail::function_traits<Fun>::arg_types>
    CoroutineContextWrap<ArgsTup> createCoroutineContext(Fun&& fun)
    {
        static_assert(detail::is_function<Fun>::value);
        static_assert(std::is_same<typename detail::function_traits<Fun>::return_type, void>::value);
        return std::make_shared<CoroutineContext<ArgsTup>>(std::forward<Fun>(fun));
    }

    namespace detail
    {
        template<typename ArgsTup, size_t... I>
        void awaitImpl(std::shared_ptr<CoroutineContext<ArgsTup>> coroPtr,
                       std::index_sequence<I...>,
                       std::tuple_element_t<I, ArgsTup>... args)
        {
            CoroutineContextBase* currentCoro = thisCoro();
            if (currentCoro == nullptr)
            {
                throw std::runtime_error{ "currentCoro is null" };
            }
            else
            {
                coroPtr->setLastCallback([currentCoro]() {
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
                ArgsTup data = std::forward_as_tuple(args...);
                currentCoro->yield([=]() {
                    coroPtr->start(data);
                });
            }
        }
    } // namespace detail

    template<typename ArgsTup, typename... Args>
    void await(CoroutineContextPtr<ArgsTup> coroPtr, Args&&... args)
    {
        detail::awaitImpl(coroPtr, std::make_index_sequence<std::tuple_size_v<ArgsTup>>{}, std::forward<Args>(args)...);
    }

    template<typename ArgsTup, typename... Args>
    void await(CoroutineContextWrap<ArgsTup> coroWrap, Args&&... args)
    {
        detail::awaitImpl(coroWrap.getCoroPtr(), std::make_index_sequence<std::tuple_size_v<ArgsTup>>{}, std::forward<Args>(args)...);
    }

    void runOn(BaseAbstractExecutor& executor)
    {
        CoroutineContextBase* coro = thisCoro();
        coro->_executor = &executor;

        coro->yield([coro]() {
            coro->_executor->post([coro]() {
                coro->resume();
            });
        });
    }

#define Coro cw::CoroMacroImpl{} +

    struct CoroMacroImpl
    {
    private:
        template<typename ArgsTup, typename Fun, size_t... I>
        std::function<void(std::tuple_element_t<I, ArgsTup>...)> makeFun(Fun&& fun, std::index_sequence<I...>)
        {
            auto coroFun = createCoroutineContext(std::forward<Fun>(fun));
            return coroFun;
        }

    public:
        template<typename Fun>
        auto operator+(Fun&& fun)
        {
            using arg_types = typename detail::function_traits<Fun>::arg_types;
            return makeFun<arg_types>(std::forward<Fun>(fun), std::make_index_sequence<std::tuple_size_v<arg_types>>{});
        }
    };

} // namespace cw