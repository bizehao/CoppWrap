
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_CONFIG_H
#define BOOST_CONTEXT_DETAIL_CONFIG_H

// required for SD-6 compile-time integer sequences
#include <utility>

#define BOOST_HAS_DECLSPEC
#define BOOST_SYMBOL_EXPORT __declspec(dllexport)
#define BOOST_SYMBOL_IMPORT __declspec(dllimport)

#  define BOOST_CONTEXT_DECL //BOOST_SYMBOL_IMPORT

#if ! defined(BOOST_CONTEXT_DECL)
# define BOOST_CONTEXT_DECL
#endif

#undef BOOST_CONTEXT_CALLDECL
#if (defined(i386) || defined(__i386__) || defined(__i386) \
     || defined(__i486__) || defined(__i586__) || defined(__i686__) \
     || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
     || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
     || defined(_M_IX86) || defined(_I86_)) && defined(BOOST_WINDOWS)
# define BOOST_CONTEXT_CALLDECL __cdecl
#else
# define BOOST_CONTEXT_CALLDECL
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
# if ! ( (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6) ) ) || \
         (defined(__clang__) && (__clang_major__ > 2 || ( __clang_major__ == 2 && __clang_minor__ > 3) ) ) )
#  error "compiler does not support segmented_stack stacks"
# endif
# define BOOST_CONTEXT_SEGMENTS 10
#endif

#if ! defined(BOOST_EXECUTION_CONTEXT)
# if defined(BOOST_USE_SEGMENTED_STACKS)
#  define BOOST_EXECUTION_CONTEXT 1
# else
#  define BOOST_EXECUTION_CONTEXT 2
# endif
#endif

// modern architectures have cachelines with 64byte length
// ARM Cortex-A15 32/64byte, Cortex-A9 16/32/64bytes
// MIPS 74K: 32byte, 4KEc: 16byte
// ist should be safe to use 64byte for all
static constexpr std::size_t cache_alignment{ 64 };
static constexpr std::size_t cacheline_length{ 64 };
// lookahead size for prefetching
static constexpr std::size_t prefetch_stride{ 4 * cacheline_length };

#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
// GNU libstdc++ 3
#  define BOOST_CONTEXT_HAS_CXXABI_H
#endif

#if defined( BOOST_CONTEXT_HAS_CXXABI_H )
# include <cxxabi.h>
#endif

#if defined(__OpenBSD__)
// stacks need mmap(2) with MAP_STACK
# define BOOST_CONTEXT_USE_MAP_STACK
#endif

#endif // BOOST_CONTEXT_DETAIL_CONFIG_H
