
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_STACK_TRAITS_H
#define BOOST_CONTEXT_STACK_TRAITS_H

#include <cstddef>

#include <noboost/context/detail/config.hpp>


namespace boost {
namespace context {

struct BOOST_CONTEXT_DECL stack_traits
{
    static bool is_unbounded() noexcept;

    static std::size_t page_size() noexcept;

    static std::size_t default_size() noexcept;

    static std::size_t minimum_size() noexcept;

    static std::size_t maximum_size() noexcept;
};

}}


#endif // BOOST_CONTEXT_STACK_TRAITS_H
