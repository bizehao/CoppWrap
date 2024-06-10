//  Boost common_factor_ct.hpp header file  ----------------------------------//

//  (C) Copyright Daryle Walker and Stephen Cleary 2001-2002.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  https://www.boost.org/LICENSE_1_0.txt)

//  See https://www.boost.org for updates, documentation, and revision history.

#ifndef BOOST_INTEGER_COMMON_FACTOR_CT_HPP
#define BOOST_INTEGER_COMMON_FACTOR_CT_HPP

#include <cstdint>

namespace boost {
namespace integer {
using static_gcd_type = uintmax_t;
//  Implementation details  --------------------------------------------------//

namespace detail {
// Build GCD with Euclid's recursive algorithm
template <static_gcd_type Value1, static_gcd_type Value2>
struct static_gcd_helper_t {
private:
    static constexpr static_gcd_type new_value1 = Value2;
    static constexpr static_gcd_type new_value2 = Value1 % Value2;

#ifndef BOOST_BORLANDC
#define BOOST_DETAIL_GCD_HELPER_VAL(Value) static_cast<static_gcd_type>(Value)
#else
    typedef static_gcd_helper_t self_type;
#define BOOST_DETAIL_GCD_HELPER_VAL(Value) (self_type::Value)
#endif

    using next_step_type = static_gcd_helper_t<BOOST_DETAIL_GCD_HELPER_VAL(new_value1),
                                               BOOST_DETAIL_GCD_HELPER_VAL(new_value2)>;

#undef BOOST_DETAIL_GCD_HELPER_VAL

public:
    static constexpr static_gcd_type value = next_step_type::value;
};

// Non-recursive case
template <static_gcd_type Value1>
struct static_gcd_helper_t<Value1, 0UL> {
    static constexpr static_gcd_type value = Value1;
};

// Build the LCM from the GCD
template <static_gcd_type Value1, static_gcd_type Value2>
struct static_lcm_helper_t {
    typedef static_gcd_helper_t<Value1, Value2> gcd_type;

    static constexpr static_gcd_type value = Value1 / gcd_type::value * Value2;
};

// Special case for zero-GCD values
template <>
struct static_lcm_helper_t<0UL, 0UL> {
    static constexpr static_gcd_type value = 0UL;
};

} // namespace detail

//  Compile-time greatest common divisor evaluator class declaration  --------//
template <static_gcd_type Value1, static_gcd_type Value2>
struct static_gcd {
    static constexpr static_gcd_type value = (detail::static_gcd_helper_t<Value1, Value2>::value);
}; // boost::integer::static_gcd

//  Compile-time least common multiple evaluator class declaration  ----------//
template <static_gcd_type Value1, static_gcd_type Value2>
struct static_lcm {
    static constexpr static_gcd_type value = (detail::static_lcm_helper_t<Value1, Value2>::value);
}; // boost::integer::static_lcm


}
} // namespace boost::integer

#endif // BOOST_INTEGER_COMMON_FACTOR_CT_HPP
