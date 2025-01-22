
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
#define BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H

#include <cstddef>

#include <noboost/context/detail/config.hpp>

// TODO:

namespace boost
{
    namespace context
    {
        namespace detail
        {

            namespace std_impl
            {
                template<class T, T... I>
                struct integer_sequence
                {
                    template<T N>
                    using append = integer_sequence<T, I..., N>;
                    static std::size_t size() { return sizeof...(I); }
                    using next = append<sizeof...(I)>;
                    using type = T;
                };

                template<class T, T Index, std::size_t N>
                struct sequence_generator
                {
                    using type = typename sequence_generator<T, Index - 1, N - 1>::type::next;
                };

                template<class T, T Index>
                struct sequence_generator<T, Index, 0ul>
                {
                    using type = integer_sequence<T>;
                };

                template<class _Ty, _Ty _Size>
                using make_integer_sequence = typename sequence_generator<_Ty, _Size, _Size>::type;

                template<size_t... _Vals>
                using index_sequence = integer_sequence<size_t, _Vals...>;

                template<size_t _Size>
                using make_index_sequence = make_integer_sequence<size_t, _Size>;

                template<class... _Types>
                using index_sequence_for = make_index_sequence<sizeof...(_Types)>;
            } // namespace std_impl

            template<std::size_t... I>
            using index_sequence = std_impl::index_sequence<I...>;
            template<std::size_t I>
            using make_index_sequence = std_impl::make_index_sequence<I>;
            template<typename... T>
            using index_sequence_for = std_impl::index_sequence_for<T...>;

        }
    }
} // namespace boost::context::detail

#endif // BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
