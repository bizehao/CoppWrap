#pragma once

#include "coro_wrap.hpp"

namespace cw {

    class AbstractExecutor : public BaseAbstractExecutor {
    public:
        virtual ~AbstractExecutor() = default;

        template<typename ArgsTup, typename... Args>
        void postCoroTask(CoroutineContextPtr<ArgsTup> coroPtr, Args&&... args);

    private:
        template<typename ArgsTup, size_t... I>
        void postCoroTaskImpl(CoroutineContextPtr<ArgsTup> coroPtr, std::index_sequence<I...>, std::tuple_element_t<I, ArgsTup>... args);
    };

    template<typename ArgsTup, typename... Args>
    inline void AbstractExecutor::postCoroTask(CoroutineContextPtr<ArgsTup> coroPtr, Args&&... args) {
        postCoroTaskImpl(coroPtr, std::make_index_sequence<std::tuple_size_v<ArgsTup>> {}, std::forward<Args>(args)...);
    }

    template<typename ArgsTup, size_t... I>
    inline void AbstractExecutor::postCoroTaskImpl(CoroutineContextPtr<ArgsTup> coroPtr,
        std::index_sequence<I...>,
        std::tuple_element_t<I, ArgsTup>... args) {
        coroPtr->_executor = this;
        ArgsTup data = std::forward_as_tuple(args...);
        std::function<void()> fun = [coroPtr, data = std::move(data)]() {
            coroPtr->start(data);
            };
        post(fun);
    }
}



