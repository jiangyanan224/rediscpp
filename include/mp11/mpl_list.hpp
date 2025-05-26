#ifndef MP11_MPL_LIST_HPP_INCLUDED
#define MP11_MPL_LIST_HPP_INCLUDED

// Copyright 2017, 2019 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <mp11/detail/mpl_common.hpp>

namespace mpl
{

template <typename Sequence>
struct sequence_tag;

template <class... T>
struct sequence_tag<mp11::mp_list<T...>>
{
    using type = aux::mp11_tag;
};

}  // namespace mpl

#endif  // #ifndef MP11_MPL_LIST_HPP_INCLUDED
