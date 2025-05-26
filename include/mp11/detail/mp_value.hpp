#ifndef MP11_DETAIL_MP_VALUE_HPP_INCLUDED
#define MP11_DETAIL_MP_VALUE_HPP_INCLUDED

// Copyright 2023 Peter Dimov
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#include <mp11/detail/config.hpp>
#include <type_traits>

#if defined(MP11_HAS_TEMPLATE_AUTO)

namespace mp11
{

template <auto A>
using mp_value = std::integral_constant<decltype(A), A>;

}  // namespace mp11

#endif  // #if defined(MP11_HAS_TEMPLATE_AUTO)

#endif  // #ifndef MP11_DETAIL_MP_VALUE_HPP_INCLUDED
