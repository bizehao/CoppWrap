#ifndef BOOST_OTHER_H
#define BOOST_OTHER_H

#include <assert.h> // .h to support old libraries w/o <cassert> - effect is the same

#define BOOST_PREVENT_MACRO_SUBSTITUTION

#ifndef BOOST_USING_STD_MIN
#define BOOST_USING_STD_MIN() using std::min
#endif

#ifndef BOOST_USING_STD_MAX
#define BOOST_USING_STD_MAX() using std::max
#endif

#define BOOST_STATIC_CONSTANT(type, assignment) static const type assignment



#undef BOOST_ASSERT
#undef BOOST_ASSERT_MSG
#undef BOOST_ASSERT_IS_VOID

#if defined(NDEBUG)
#define BOOST_ASSERT(expr) ((void)0)
#define BOOST_ASSERT_MSG(expr, msg) ((void)0)
#define BOOST_ASSERT_IS_VOID
#else
#define BOOST_ASSERT(expr) assert(expr)
#define BOOST_ASSERT_MSG(expr, msg) assert((expr) && (msg))
#endif

#endif

