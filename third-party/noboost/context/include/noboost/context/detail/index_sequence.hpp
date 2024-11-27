
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
#define BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H

#include <cstddef>

#include <noboost/context/detail/config.hpp>

// TODO:

namespace boost {
namespace context {
namespace detail {

namespace std_impl {
template <class _Ty, _Ty... _Vals>
struct integer_sequence { // sequence of integer parameters
    static_assert(is_integral_v<_Ty>, "integer_sequence<T, I...> requires T to be an integral type.");

    using value_type = _Ty;

    static constexpr size_t size() noexcept {
        return sizeof...(_Vals);
    }
};

template <class _Ty, _Ty _Size>
using make_integer_sequence = __make_integer_seq<integer_sequence, _Ty, _Size>;

template <size_t... _Vals>
using index_sequence = integer_sequence<size_t, _Vals...>;

template <size_t _Size>
using make_index_sequence = make_integer_sequence<size_t, _Size>;

template <class... _Types>
using index_sequence_for = make_index_sequence<sizeof...(_Types)>;
} // namespace std_impl

template <std::size_t... I>
using index_sequence = std_impl::index_sequence<I...>;
template <std::size_t I>
using make_index_sequence = std_impl::make_index_sequence<I>;
template <typename... T>
using index_sequence_for = std_impl::index_sequence_for<T...>;

}}} // namespace boost::context::detail

#endif // BOOST_CONTEXT_DETAIL_INDEX_SEQUENCE_H
