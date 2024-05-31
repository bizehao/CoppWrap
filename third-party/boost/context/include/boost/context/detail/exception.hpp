
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_EXCEPTION_H
#define BOOST_CONTEXT_DETAIL_EXCEPTION_H

#include <cassert>

#include <boost/context/detail/fcontext.hpp>


namespace boost {
namespace context {
namespace detail {

struct forced_unwind {
    fcontext_t  fctx{ nullptr };

    forced_unwind() = default;

    forced_unwind( fcontext_t fctx_) :
        fctx( fctx_) {
    }
};

}}}


#endif // BOOST_CONTEXT_DETAIL_EXCEPTION_H