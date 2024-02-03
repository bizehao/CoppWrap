#include <type_traits>

namespace cw {

    namespace detail {

        /////////////////////////////////////////////////////////////////////////////////////

        template<typename T>
        struct is_function_ptr :
            std::integral_constant<bool, std::is_pointer<T>::value&& std::is_function<std::remove_pointer_t<T>>::value> {};

        /////////////////////////////////////////////////////////////////////////////////////
        // snippet provided by K-ballo
        struct helper {
            void operator()(...);
        };

        template<typename T>
        struct helper_composed : T, helper {};

        template<void (helper::*)(...)>
        struct member_function_holder {};

        template<typename T, typename Ambiguous = member_function_holder<&helper::operator()>>
        struct is_functor_impl : std::true_type {};

        template<typename T>
        struct is_functor_impl<T, member_function_holder<&helper_composed<T>::operator()>> : std::false_type {};

        /*!
         * \brief Returns true whether the given type T is a functor.
         *        i.e. func(...); That can be free function, lambdas or function objects.
         */
        template<typename T>
        struct is_functor : std::conditional_t<std::is_class<T>::value, is_functor_impl<T>, std::false_type> {};

        template<typename R, typename... Args>
        struct is_functor<R(*)(Args...)> : std::true_type {};

        template<typename R, typename... Args>
        struct is_functor<R(&)(Args...)> : std::true_type {};

        template<typename R, typename... Args>
        struct is_functor<R(*)(Args...) noexcept> : std::true_type {};

        template<typename R, typename... Args>
        struct is_functor<R(&)(Args...) noexcept> : std::true_type {};

        /////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////////////////////////////////////////////////////////

        template<typename T>
        struct function_traits : function_traits<decltype(&T::operator())> {};

        template<typename R, typename... Args>
        struct function_traits<R(Args...)> {
            static constexpr size_t arg_count = sizeof...(Args);

            using return_type = R;
            using arg_types = std::tuple<Args...>;
        };

        template<typename R, typename... Args>
        struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

        template<typename R, typename... Args>
        struct function_traits<R(&)(Args...)> : function_traits<R(Args...)> {};

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) volatile> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) const volatile> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename... Args>
        struct function_traits<R(*)(Args...) noexcept> : function_traits<R(Args...)> {};

        template<typename R, typename... Args>
        struct function_traits<R(&)(Args...) noexcept> : function_traits<R(Args...)> {};

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) noexcept> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) const noexcept> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) volatile noexcept> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename R, typename C, typename... Args>
        struct function_traits<R(C::*)(Args...) const volatile noexcept> : function_traits<R(Args...)> {
            using class_type = C;
        };

        template<typename T>
        struct function_traits<std::function<T>> : function_traits<T> {};

        /////////////////////////////////////////////////////////////////////////////////////
        // use it like e.g:
        // param_types<F, 0>::type

        template<typename F, size_t Index>
        struct param_types {
            using type = typename std::tuple_element<Index, typename function_traits<F>::arg_types>::type;
        };

        template<typename F, size_t Index>
        using param_types_t = typename param_types<F, Index>::type;

        /////////////////////////////////////////////////////////////////////////////////////

        template<typename F>
        struct is_void_func :
            std::conditional_t<std::is_same<typename function_traits<F>::return_type, void>::value, std::true_type, std::false_type> {};

        /////////////////////////////////////////////////////////////////////////////////////
        // returns an std::true_type, when the given type F is a function type; otherwise an std::false_type.
        template<typename F>
        using is_function =
            std::integral_constant<bool, std::is_member_function_pointer<F>::value || std::is_function<F>::value || is_functor<F>::value>;

        /////////////////////////////////////////////////////////////////////////////////////

        template<typename V>
        struct ForwardValue {
            template<typename T>
            void operator()(T*& ptr, V& v) {
                *ptr = v;
            }
        };

        template<typename V>
        struct ForwardValue<V&> {
            template<typename T>
            void operator()(T*& ptr, V& v) {
                ptr = &v;
            }
        };

    }  // end namespace detail

}
