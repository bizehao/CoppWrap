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
            _enter_source = std::move(_output_source).resume();
            finish_callback();
        }

        // 恢复到切入点的时侯 call fun
        /*void yield(std::function<void()> fun = nullptr)
        {
            thisCoro() = nullptr;
            _source = std::move(_source).resume_with([fun = std::move(fun)](ctx::fiber&& fc) {
                fun();
                return std::move(fc);
            });
            finish_callback();
        }*/

        void yield()
        {
            thisCoro() = nullptr;
            _output_source = std::move(_enter_source).resume();
            finish_callback();
        }

        void yield(std::function<ctx::fiber(ctx::fiber&&)> fun)
        {
            thisCoro() = nullptr;
            _output_source = std::move(_enter_source).resume_with(std::move(fun));
            finish_callback();
        }

        bool isFinished()
        {
            return _is_finished;
        }

        void setLastCallback(std::function<void()> callback)
        {
            _last_call = callback;
        }

        BaseAbstractExecutor* getExecutor()
        {
            return _executor;
        }

        void destory()
        {
            _output_source = {};
            finish_callback();
        }

        void setEnterFiber(ctx::fiber&& f)
        {
            _enter_source = std::move(f);
        }

        void setOutputFiber(ctx::fiber&& f)
        {
            _output_source = std::move(f);
        }

        void setLastFiber(ctx::fiber&& f)
        {
            _last_resume = std::move(f);
        }

        ctx::fiber getLastFiber()
        {
            return std::move(_last_resume);
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

        ctx::fiber _enter_source; //进入
        ctx::fiber _output_source; //出去
        ctx::fiber _last_resume;
        BaseAbstractExecutor* _executor{ nullptr };
        bool _is_finished{ false };
        std::function<void()> _last_call;
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
            if (_output_source)
            {
                throw std::runtime_error{ "Started multiple times" };
            }
            _output_source = ctx::fiber{ [self = getSelf(), &tup](ctx::fiber&& fiber) {
                self->_enter_source = std::move(fiber);
                self->_prolong_life = self;
                self->invoke(self->_fun, tup, std::make_index_sequence<std::tuple_size_v<ArgsTup>>{});
                self->_is_finished = true;
                return std::move(self->_enter_source);
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
                coroPtr->setLastCallback([coroPtr, currentCoro]() {
                    if (currentCoro->getExecutor() != nullptr)
                    {
                        currentCoro->getExecutor()->post([currentCoro, coroPtr]() {
                            currentCoro->setOutputFiber(coroPtr->getLastFiber());
                            currentCoro->resume();
                        });
                    }
                    else
                    {
                        currentCoro->setOutputFiber(coroPtr->getLastFiber());
                        currentCoro->resume();
                    }
                });
                ArgsTup data = std::forward_as_tuple(args...);

                currentCoro->yield([=](ctx::fiber&& f) {
                    currentCoro->setLastFiber(std::move(f));
                    coroPtr->start(data);
                    return ctx::fiber{};
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

        coro->yield([coro, &executor](ctx::fiber&& f) {
            auto mw_f = folly::makeMoveWrapper(std::move(f));
            executor.post([mw_f, coro]() mutable {
                thisCoro() = coro;
                coro->setEnterFiber(mw_f.move().resume());
            });
            return ctx::fiber{};
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