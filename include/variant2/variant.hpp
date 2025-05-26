#ifndef VARIANT2_VARIANT_HPP_INCLUDED
#define VARIANT2_VARIANT_HPP_INCLUDED

// Copyright 2017-2019 Peter Dimov.
//
// Distributed under the Boost Software License, Version 1.0.
//
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#if defined(_MSC_VER) && _MSC_VER < 1910
#pragma warning(push)
#pragma warning(disable : 4521 4522)  // multiple copy operators
#endif

#include <mp11.hpp>
#include <cassert>
#include <source_location>
#include <cstddef>
#include <type_traits>
#include <exception>
#include <utility>
#include <typeindex>  // std::hash
#include <iosfwd>
#include <cstdint>
#include <cerrno>
#include <limits>

//

template <class T>
struct hash;

namespace variant2
{

// bad_variant_access

class bad_variant_access : public std::exception
{
public:
    bad_variant_access() noexcept
    {
    }

    char const* what() const noexcept
    {
        return "bad_variant_access";
    }
};

namespace detail
{

[[noreturn]] inline void throw_bad_variant_access()
{
    throw bad_variant_access();
}

}  // namespace detail

constexpr bool operator<(std::monostate, std::monostate) noexcept
{
    return false;
}
constexpr bool operator>(std::monostate, std::monostate) noexcept
{
    return false;
}
constexpr bool operator<=(std::monostate, std::monostate) noexcept
{
    return true;
}
constexpr bool operator>=(std::monostate, std::monostate) noexcept
{
    return true;
}
constexpr bool operator==(std::monostate, std::monostate) noexcept
{
    return true;
}
constexpr bool operator!=(std::monostate, std::monostate) noexcept
{
    return false;
}

// variant_size

template <class T>
struct variant_size
{
};

template <class T>
struct variant_size<T const> : variant_size<T>
{
};

template <class T>
struct variant_size<T volatile> : variant_size<T>
{
};

template <class T>
struct variant_size<T const volatile> : variant_size<T>
{
};

template <class T>
struct variant_size<T&> : variant_size<T>
{
};

template <class T>
struct variant_size<T&&> : variant_size<T>
{
};

template <class T> /*inline*/ constexpr std::size_t variant_size_v = variant_size<T>::value;

template <class... T>
struct variant_size<std::variant<T...>> : mp11::mp_size<std::variant<T...>>
{
};

// variant_alternative

template <std::size_t I, class T>
struct variant_alternative;

template <std::size_t I, class T>
using variant_alternative_t = typename variant_alternative<I, T>::type;

namespace detail
{

template <class I, class T, class Q>
using var_alt_impl = mp11::mp_invoke_q<Q, variant_alternative_t<I::value, T>>;

}  // namespace detail

template <std::size_t I, class T>
struct variant_alternative
{
};

template <std::size_t I, class T>
struct variant_alternative<I, T const> : mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_const>>
{
};

template <std::size_t I, class T>
struct variant_alternative<I, T volatile> : mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_volatile>>
{
};

template <std::size_t I, class T>
struct variant_alternative<I, T const volatile> : mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_cv>>
{
};

template <std::size_t I, class T>
struct variant_alternative<I, T&> : mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_lvalue_reference>>
{
};

template <std::size_t I, class T>
struct variant_alternative<I, T&&> : mp11::mp_defer<detail::var_alt_impl, mp11::mp_size_t<I>, T, mp11::mp_quote_trait<std::add_rvalue_reference>>
{
};

template <std::size_t I, class... T>
struct variant_alternative<I, std::variant<T...>> : mp11::mp_defer<mp11::mp_at, std::variant<T...>, mp11::mp_size_t<I>>
{
};

// variant_npos

constexpr std::size_t variant_npos = ~static_cast<std::size_t>(0);

// holds_alternative

template <class U, class... T>
constexpr bool holds_alternative(std::variant<T...> const& v) noexcept
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");
    return v.index() == mp11::mp_find<std::variant<T...>, U>::value;
}

// get (index)

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>>& get(std::variant<T...>& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");
    return (v.index() != I ? detail::throw_bad_variant_access() : (void)0), v._get_impl(mp11::mp_size_t<I>());
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>>&& get(std::variant<T...>&& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    return (v.index() != I ? detail::throw_bad_variant_access() : (void)0), std::move(v._get_impl(mp11::mp_size_t<I>()));
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>> const& get(std::variant<T...> const& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");
    return (v.index() != I ? detail::throw_bad_variant_access() : (void)0), v._get_impl(mp11::mp_size_t<I>());
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>> const&& get(std::variant<T...> const&& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    return (v.index() != I ? detail::throw_bad_variant_access() : (void)0), std::move(v._get_impl(mp11::mp_size_t<I>()));
}

// unsafe_get

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>>& unsafe_get(std::variant<T...>& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    assert(v.index() == I);

    return v._get_impl(mp11::mp_size_t<I>());
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>>&& unsafe_get(std::variant<T...>&& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    assert(v.index() == I);

    return std::move(v._get_impl(mp11::mp_size_t<I>()));
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>> const& unsafe_get(std::variant<T...> const& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    assert(v.index() == I);

    return v._get_impl(mp11::mp_size_t<I>());
}

template <std::size_t I, class... T>
constexpr variant_alternative_t<I, std::variant<T...>> const&& unsafe_get(std::variant<T...> const&& v)
{
    static_assert(I < sizeof...(T), "Index out of bounds");

    assert(v.index() == I);

    return std::move(v._get_impl(mp11::mp_size_t<I>()));
}

// get (type)

template <class U, class... T>
constexpr U& get(std::variant<T...>& v)
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return (v.index() != I::value ? detail::throw_bad_variant_access() : (void)0), v._get_impl(I());
}

template <class U, class... T>
constexpr U&& get(std::variant<T...>&& v)
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return (v.index() != I::value ? detail::throw_bad_variant_access() : (void)0), std::move(v._get_impl(I()));
}

template <class U, class... T>
constexpr U const& get(std::variant<T...> const& v)
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return (v.index() != I::value ? detail::throw_bad_variant_access() : (void)0), v._get_impl(I());
}

template <class U, class... T>
constexpr U const&& get(std::variant<T...> const&& v)
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return (v.index() != I::value ? detail::throw_bad_variant_access() : (void)0), std::move(v._get_impl(I()));
}

// get_if

template <std::size_t I, class... T>
constexpr typename std::add_pointer<variant_alternative_t<I, std::variant<T...>>>::type get_if(std::variant<T...>* v) noexcept
{
    static_assert(I < sizeof...(T), "Index out of bounds");
    return v && v->index() == I ? &v->_get_impl(mp11::mp_size_t<I>()) : 0;
}

template <std::size_t I, class... T>
constexpr typename std::add_pointer<const variant_alternative_t<I, std::variant<T...>>>::type get_if(std::variant<T...> const* v) noexcept
{
    static_assert(I < sizeof...(T), "Index out of bounds");
    return v && v->index() == I ? &v->_get_impl(mp11::mp_size_t<I>()) : 0;
}

template <class U, class... T>
constexpr typename std::add_pointer<U>::type get_if(std::variant<T...>* v) noexcept
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return v && v->index() == I::value ? &v->_get_impl(I()) : 0;
}

template <class U, class... T>
constexpr typename std::add_pointer<U const>::type get_if(std::variant<T...> const* v) noexcept
{
    static_assert(mp11::mp_count<std::variant<T...>, U>::value == 1, "The type must occur exactly once in the list of std::variant alternatives");

    using I = mp11::mp_find<std::variant<T...>, U>;

    return v && v->index() == I::value ? &v->_get_impl(I()) : 0;
}

//

namespace detail
{

// trivially_*

using std::is_trivially_copy_assignable;
using std::is_trivially_copy_constructible;
using std::is_trivially_move_assignable;
using std::is_trivially_move_constructible;

// variant_storage

template <class D, class... T>
union variant_storage_impl;

template <class... T>
using variant_storage = variant_storage_impl<mp11::mp_all<std::is_trivially_destructible<T>...>, T...>;

template <class D>
union variant_storage_impl<D>
{
};

// not all trivially destructible
template <class T1, class... T>
union variant_storage_impl<mp11::mp_false, T1, T...>
{
    T1 first_;
    variant_storage<T...> rest_;

    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<0>, A&&... a) : first_(std::forward<A>(a)...)
    {
    }

    template <std::size_t I, class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<I>, A&&... a) : rest_(mp11::mp_size_t<I - 1>(), std::forward<A>(a)...)
    {
    }

    ~variant_storage_impl()
    {
    }

    template <class... A>
    void emplace(mp11::mp_size_t<0>, A&&... a)
    {
        ::new (&first_) T1(std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    void emplace(mp11::mp_size_t<I>, A&&... a)
    {
        rest_.emplace(mp11::mp_size_t<I - 1>(), std::forward<A>(a)...);
    }

    constexpr T1& get(mp11::mp_size_t<0>) noexcept
    {
        return first_;
    }
    constexpr T1 const& get(mp11::mp_size_t<0>) const noexcept
    {
        return first_;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 1>& get(mp11::mp_size_t<I>) noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 1>());
    }
    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 1> const& get(mp11::mp_size_t<I>) const noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 1>());
    }
};

template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... T>
union variant_storage_impl<mp11::mp_false, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T...>
{
    T0 t0_;
    T1 t1_;
    T2 t2_;
    T3 t3_;
    T4 t4_;
    T5 t5_;
    T6 t6_;
    T7 t7_;
    T8 t8_;
    T9 t9_;

    variant_storage<T...> rest_;

    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<0>, A&&... a) : t0_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<1>, A&&... a) : t1_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<2>, A&&... a) : t2_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<3>, A&&... a) : t3_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<4>, A&&... a) : t4_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<5>, A&&... a) : t5_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<6>, A&&... a) : t6_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<7>, A&&... a) : t7_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<8>, A&&... a) : t8_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<9>, A&&... a) : t9_(std::forward<A>(a)...)
    {
    }

    template <std::size_t I, class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<I>, A&&... a) : rest_(mp11::mp_size_t<I - 10>(), std::forward<A>(a)...)
    {
    }

    ~variant_storage_impl()
    {
    }

    template <class... A>
    void emplace(mp11::mp_size_t<0>, A&&... a)
    {
        ::new (&t0_) T0(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<1>, A&&... a)
    {
        ::new (&t1_) T1(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<2>, A&&... a)
    {
        ::new (&t2_) T2(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<3>, A&&... a)
    {
        ::new (&t3_) T3(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<4>, A&&... a)
    {
        ::new (&t4_) T4(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<5>, A&&... a)
    {
        ::new (&t5_) T5(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<6>, A&&... a)
    {
        ::new (&t6_) T6(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<7>, A&&... a)
    {
        ::new (&t7_) T7(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<8>, A&&... a)
    {
        ::new (&t8_) T8(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace(mp11::mp_size_t<9>, A&&... a)
    {
        ::new (&t9_) T9(std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    void emplace(mp11::mp_size_t<I>, A&&... a)
    {
        rest_.emplace(mp11::mp_size_t<I - 10>(), std::forward<A>(a)...);
    }

    constexpr T0& get(mp11::mp_size_t<0>) noexcept
    {
        return t0_;
    }
    constexpr T0 const& get(mp11::mp_size_t<0>) const noexcept
    {
        return t0_;
    }

    constexpr T1& get(mp11::mp_size_t<1>) noexcept
    {
        return t1_;
    }
    constexpr T1 const& get(mp11::mp_size_t<1>) const noexcept
    {
        return t1_;
    }

    constexpr T2& get(mp11::mp_size_t<2>) noexcept
    {
        return t2_;
    }
    constexpr T2 const& get(mp11::mp_size_t<2>) const noexcept
    {
        return t2_;
    }

    constexpr T3& get(mp11::mp_size_t<3>) noexcept
    {
        return t3_;
    }
    constexpr T3 const& get(mp11::mp_size_t<3>) const noexcept
    {
        return t3_;
    }

    constexpr T4& get(mp11::mp_size_t<4>) noexcept
    {
        return t4_;
    }
    constexpr T4 const& get(mp11::mp_size_t<4>) const noexcept
    {
        return t4_;
    }

    constexpr T5& get(mp11::mp_size_t<5>) noexcept
    {
        return t5_;
    }
    constexpr T5 const& get(mp11::mp_size_t<5>) const noexcept
    {
        return t5_;
    }

    constexpr T6& get(mp11::mp_size_t<6>) noexcept
    {
        return t6_;
    }
    constexpr T6 const& get(mp11::mp_size_t<6>) const noexcept
    {
        return t6_;
    }

    constexpr T7& get(mp11::mp_size_t<7>) noexcept
    {
        return t7_;
    }
    constexpr T7 const& get(mp11::mp_size_t<7>) const noexcept
    {
        return t7_;
    }

    constexpr T8& get(mp11::mp_size_t<8>) noexcept
    {
        return t8_;
    }
    constexpr T8 const& get(mp11::mp_size_t<8>) const noexcept
    {
        return t8_;
    }

    constexpr T9& get(mp11::mp_size_t<9>) noexcept
    {
        return t9_;
    }
    constexpr T9 const& get(mp11::mp_size_t<9>) const noexcept
    {
        return t9_;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 10>& get(mp11::mp_size_t<I>) noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 10>());
    }
    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 10> const& get(mp11::mp_size_t<I>) const noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 10>());
    }
};

// all trivially destructible
template <class T1, class... T>
union variant_storage_impl<mp11::mp_true, T1, T...>
{
    T1 first_;
    variant_storage<T...> rest_;

    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<0>, A&&... a) : first_(std::forward<A>(a)...)
    {
    }

    template <std::size_t I, class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<I>, A&&... a) : rest_(mp11::mp_size_t<I - 1>(), std::forward<A>(a)...)
    {
    }

    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<0>, A&&... a)
    {
        ::new (&first_) T1(std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    constexpr void emplace_impl(mp11::mp_false, mp11::mp_size_t<I>, A&&... a)
    {
        rest_.emplace(mp11::mp_size_t<I - 1>(), std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    constexpr void emplace_impl(mp11::mp_true, mp11::mp_size_t<I>, A&&... a)
    {
#if __GNUC__ >= 7
#pragma GCC diagnostic push
// False positive in at least GCC 7 and GCC 10 ASAN triggered by monostate (via result<void>)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#if __GNUC__ >= 12
// False positive in at least GCC 12 and GCC 13 ASAN and -Og triggered by monostate (via result<void>)
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
#endif
        *this = variant_storage_impl(mp11::mp_size_t<I>(), std::forward<A>(a)...);

#if __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
    }

    template <std::size_t I, class... A>
    constexpr void emplace(mp11::mp_size_t<I>, A&&... a)
    {
        this->emplace_impl(mp11::mp_all<detail::is_trivially_move_assignable<T1>, detail::is_trivially_move_assignable<T>...>(), mp11::mp_size_t<I>(),
                           std::forward<A>(a)...);
    }

    constexpr T1& get(mp11::mp_size_t<0>) noexcept
    {
        return first_;
    }
    constexpr T1 const& get(mp11::mp_size_t<0>) const noexcept
    {
        return first_;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 1>& get(mp11::mp_size_t<I>) noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 1>());
    }
    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 1> const& get(mp11::mp_size_t<I>) const noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 1>());
    }
};

template <class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class... T>
union variant_storage_impl<mp11::mp_true, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T...>
{
    T0 t0_;
    T1 t1_;
    T2 t2_;
    T3 t3_;
    T4 t4_;
    T5 t5_;
    T6 t6_;
    T7 t7_;
    T8 t8_;
    T9 t9_;

    variant_storage<T...> rest_;

    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<0>, A&&... a) : t0_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<1>, A&&... a) : t1_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<2>, A&&... a) : t2_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<3>, A&&... a) : t3_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<4>, A&&... a) : t4_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<5>, A&&... a) : t5_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<6>, A&&... a) : t6_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<7>, A&&... a) : t7_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<8>, A&&... a) : t8_(std::forward<A>(a)...)
    {
    }
    template <class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<9>, A&&... a) : t9_(std::forward<A>(a)...)
    {
    }

    template <std::size_t I, class... A>
    constexpr variant_storage_impl(mp11::mp_size_t<I>, A&&... a) : rest_(mp11::mp_size_t<I - 10>(), std::forward<A>(a)...)
    {
    }

    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<0>, A&&... a)
    {
        ::new (&t0_) T0(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<1>, A&&... a)
    {
        ::new (&t1_) T1(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<2>, A&&... a)
    {
        ::new (&t2_) T2(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<3>, A&&... a)
    {
        ::new (&t3_) T3(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<4>, A&&... a)
    {
        ::new (&t4_) T4(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<5>, A&&... a)
    {
        ::new (&t5_) T5(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<6>, A&&... a)
    {
        ::new (&t6_) T6(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<7>, A&&... a)
    {
        ::new (&t7_) T7(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<8>, A&&... a)
    {
        ::new (&t8_) T8(std::forward<A>(a)...);
    }
    template <class... A>
    void emplace_impl(mp11::mp_false, mp11::mp_size_t<9>, A&&... a)
    {
        ::new (&t9_) T9(std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    constexpr void emplace_impl(mp11::mp_false, mp11::mp_size_t<I>, A&&... a)
    {
        rest_.emplace(mp11::mp_size_t<I - 10>(), std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    constexpr void emplace_impl(mp11::mp_true, mp11::mp_size_t<I>, A&&... a)
    {
        *this = variant_storage_impl(mp11::mp_size_t<I>(), std::forward<A>(a)...);
    }

    template <std::size_t I, class... A>
    constexpr void emplace(mp11::mp_size_t<I>, A&&... a)
    {
        this->emplace_impl(
            mp11::mp_all<detail::is_trivially_move_assignable<T0>, detail::is_trivially_move_assignable<T1>, detail::is_trivially_move_assignable<T2>,
                         detail::is_trivially_move_assignable<T3>, detail::is_trivially_move_assignable<T4>, detail::is_trivially_move_assignable<T5>,
                         detail::is_trivially_move_assignable<T6>, detail::is_trivially_move_assignable<T7>, detail::is_trivially_move_assignable<T8>,
                         detail::is_trivially_move_assignable<T9>, detail::is_trivially_move_assignable<T>...>(),
            mp11::mp_size_t<I>(), std::forward<A>(a)...);
    }

    constexpr T0& get(mp11::mp_size_t<0>) noexcept
    {
        return t0_;
    }
    constexpr T0 const& get(mp11::mp_size_t<0>) const noexcept
    {
        return t0_;
    }

    constexpr T1& get(mp11::mp_size_t<1>) noexcept
    {
        return t1_;
    }
    constexpr T1 const& get(mp11::mp_size_t<1>) const noexcept
    {
        return t1_;
    }

    constexpr T2& get(mp11::mp_size_t<2>) noexcept
    {
        return t2_;
    }
    constexpr T2 const& get(mp11::mp_size_t<2>) const noexcept
    {
        return t2_;
    }

    constexpr T3& get(mp11::mp_size_t<3>) noexcept
    {
        return t3_;
    }
    constexpr T3 const& get(mp11::mp_size_t<3>) const noexcept
    {
        return t3_;
    }

    constexpr T4& get(mp11::mp_size_t<4>) noexcept
    {
        return t4_;
    }
    constexpr T4 const& get(mp11::mp_size_t<4>) const noexcept
    {
        return t4_;
    }

    constexpr T5& get(mp11::mp_size_t<5>) noexcept
    {
        return t5_;
    }
    constexpr T5 const& get(mp11::mp_size_t<5>) const noexcept
    {
        return t5_;
    }

    constexpr T6& get(mp11::mp_size_t<6>) noexcept
    {
        return t6_;
    }
    constexpr T6 const& get(mp11::mp_size_t<6>) const noexcept
    {
        return t6_;
    }

    constexpr T7& get(mp11::mp_size_t<7>) noexcept
    {
        return t7_;
    }
    constexpr T7 const& get(mp11::mp_size_t<7>) const noexcept
    {
        return t7_;
    }

    constexpr T8& get(mp11::mp_size_t<8>) noexcept
    {
        return t8_;
    }
    constexpr T8 const& get(mp11::mp_size_t<8>) const noexcept
    {
        return t8_;
    }

    constexpr T9& get(mp11::mp_size_t<9>) noexcept
    {
        return t9_;
    }
    constexpr T9 const& get(mp11::mp_size_t<9>) const noexcept
    {
        return t9_;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 10>& get(mp11::mp_size_t<I>) noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 10>());
    }
    template <std::size_t I>
    constexpr mp11::mp_at_c<mp11::mp_list<T...>, I - 10> const& get(mp11::mp_size_t<I>) const noexcept
    {
        return rest_.get(mp11::mp_size_t<I - 10>());
    }
};

// resolve_overload_*

template <class... T>
struct overload;

template <>
struct overload<>
{
    void operator()() const;
};

template <class T1, class... T>
struct overload<T1, T...> : overload<T...>
{
    using overload<T...>::operator();
    mp11::mp_identity<T1> operator()(T1) const;
};

template <class U, class... T>
using resolve_overload_type = typename decltype(overload<T...>()(std::declval<U>()))::type;

template <class U, class... T>
using resolve_overload_index = mp11::mp_find<mp11::mp_list<T...>, resolve_overload_type<U, T...>>;

// index_type

template <std::size_t N>
using get_smallest_unsigned_type = mp11::mp_cond<

    mp11::mp_bool<N <= (std::numeric_limits<unsigned char>::max)()>, unsigned char, mp11::mp_bool<N <= (std::numeric_limits<unsigned short>::max)()>,
    unsigned short, mp11::mp_true, unsigned

    >;

template <bool Double, class... T>
using get_index_type = get_smallest_unsigned_type<(Double + 1) * sizeof...(T)>;

// variant_base

template <bool is_trivially_destructible, bool is_single_buffered, class... T>
struct variant_base_impl;
template <class... T>
using variant_base =
    variant_base_impl<mp11::mp_all<std::is_trivially_destructible<T>...>::value, mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value, T...>;

struct none
{
};

// trivially destructible, single buffered
template <class... T>
struct variant_base_impl<true, true, T...>
{
    using index_type = get_index_type<false, T...>;

    variant_storage<none, T...> st_;
    index_type ix_;

    constexpr variant_base_impl() : st_(mp11::mp_size_t<0>()), ix_(0)
    {
    }

    template <class I, class... A>
    constexpr explicit variant_base_impl(I, A&&... a) : st_(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...), ix_(I::value + 1)
    {
    }

    // requires: ix_ == 0
    template <class I, class... A>
    void _replace(I, A&&... a)
    {
        ::new (&st_) variant_storage<none, T...>(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...);

        static_assert(I::value + 1 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = I::value + 1;
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ - 1;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I>& _get_impl(mp11::mp_size_t<I>) noexcept
    {
        std::size_t const J = I + 1;

        assert(ix_ == J);

        return st_.get(mp11::mp_size_t<J>());
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I> const& _get_impl(mp11::mp_size_t<I>) const noexcept
    {
        // std::size_t const J = I+1;

        assert(ix_ == I + 1);

        return st_.get(mp11::mp_size_t<I + 1>());
    }

    template <std::size_t J, class U, class... A>
    constexpr void emplace_impl(mp11::mp_true, A&&... a)
    {
        static_assert(std::is_nothrow_constructible<U, A&&...>::value, "Logic error: U must be nothrow constructible from A&&...");

        st_.emplace(mp11::mp_size_t<J>(), std::forward<A>(a)...);

        static_assert(J <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = J;
    }

    template <std::size_t J, class U, class... A>
    constexpr void emplace_impl(mp11::mp_false, A&&... a)
    {
        static_assert(std::is_nothrow_move_constructible<U>::value, "Logic error: U must be nothrow move constructible");

        U tmp(std::forward<A>(a)...);

        st_.emplace(mp11::mp_size_t<J>(), std::move(tmp));

        static_assert(J <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = J;
    }

    template <std::size_t I, class... A>
    constexpr void emplace(A&&... a)
    {
        std::size_t const J = I + 1;
        using U = mp11::mp_at_c<std::variant<T...>, I>;

        this->emplace_impl<J, U>(std::is_nothrow_constructible<U, A&&...>(), std::forward<A>(a)...);
    }

    static constexpr bool uses_double_storage() noexcept
    {
        return false;
    }
};

// trivially destructible, double buffered
template <class... T>
struct variant_base_impl<true, false, T...>
{
    using index_type = get_index_type<true, T...>;

    variant_storage<none, T...> st_[2];
    index_type ix_;

    constexpr variant_base_impl() : st_ { { mp11::mp_size_t<0>() }, { mp11::mp_size_t<0>() } }, ix_(0)
    {
    }

    template <class I, class... A>
    constexpr explicit variant_base_impl(I, A&&... a)
        : st_ { { mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... }, { mp11::mp_size_t<0>() } }, ix_((I::value + 1) * 2)
    {
    }

    // requires: ix_ == 0
    template <class I, class... A>
    void _replace(I, A&&... a)
    {
        ::new (&st_[0]) variant_storage<none, T...>(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...);

        static_assert((I::value + 1) * 2 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = (I::value + 1) * 2;
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ / 2 - 1;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I>& _get_impl(mp11::mp_size_t<I>) noexcept
    {
        assert(index() == I);

        std::size_t const J = I + 1;

        constexpr mp11::mp_size_t<J> j {};
        return st_[ix_ & 1].get(j);
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I> const& _get_impl(mp11::mp_size_t<I>) const noexcept
    {
        assert(index() == I);

        // std::size_t const J = I+1;
        // constexpr mp_size_t<J> j{};

        return st_[ix_ & 1].get(mp11::mp_size_t<I + 1>());
    }

    template <std::size_t I, class... A>
    constexpr void emplace(A&&... a)
    {
        std::size_t const J = I + 1;

        unsigned i2 = 1 - (ix_ & 1);

        st_[i2].emplace(mp11::mp_size_t<J>(), std::forward<A>(a)...);

        static_assert(J * 2 + 1 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = static_cast<index_type>(J * 2 + i2);
    }

    static constexpr bool uses_double_storage() noexcept
    {
        return true;
    }
};

// not trivially destructible, single buffered
template <class... T>
struct variant_base_impl<false, true, T...>
{
    using index_type = get_index_type<false, T...>;

    variant_storage<none, T...> st_;
    index_type ix_;

    constexpr variant_base_impl() : st_(mp11::mp_size_t<0>()), ix_(0)
    {
    }

    template <class I, class... A>
    constexpr explicit variant_base_impl(I, A&&... a) : st_(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...), ix_(I::value + 1)
    {
    }

    // requires: ix_ == 0
    template <class I, class... A>
    void _replace(I, A&&... a)
    {
        ::new (&st_) variant_storage<none, T...>(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...);

        static_assert(I::value + 1 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = I::value + 1;
    }

    //[&]( auto I ){
    //    using U = mp_at_c<mp_list<none, T...>, I>;
    //    st1_.get( I ).~U();
    //}

    struct _destroy_L1
    {
        variant_base_impl* this_;

        template <class I>
        void operator()(I) const noexcept
        {
            using U = mp11::mp_at<mp11::mp_list<none, T...>, I>;
            this_->st_.get(I()).~U();
        }
    };

    void _destroy() noexcept
    {
        if (ix_ > 0)
        {
            mp11::mp_with_index<1 + sizeof...(T)>(ix_, _destroy_L1 { this });
        }
    }

    ~variant_base_impl() noexcept
    {
        _destroy();
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ - 1;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I>& _get_impl(mp11::mp_size_t<I>) noexcept
    {
        std::size_t const J = I + 1;

        assert(ix_ == J);

        return st_.get(mp11::mp_size_t<J>());
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I> const& _get_impl(mp11::mp_size_t<I>) const noexcept
    {
        // std::size_t const J = I+1;

        assert(ix_ == I + 1);

        return st_.get(mp11::mp_size_t<I + 1>());
    }

    template <std::size_t I, class... A>
    void emplace(A&&... a)
    {
        std::size_t const J = I + 1;

        using U = mp11::mp_at_c<std::variant<T...>, I>;

        static_assert(std::is_nothrow_move_constructible<U>::value, "Logic error: U must be nothrow move constructible");

        U tmp(std::forward<A>(a)...);

        _destroy();

        st_.emplace(mp11::mp_size_t<J>(), std::move(tmp));

        static_assert(J <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = J;
    }

    static constexpr bool uses_double_storage() noexcept
    {
        return false;
    }
};

// not trivially destructible, double buffered
template <class... T>
struct variant_base_impl<false, false, T...>
{
    using index_type = get_index_type<true, T...>;

    variant_storage<none, T...> st_[2];
    index_type ix_;

    constexpr variant_base_impl() : st_ { { mp11::mp_size_t<0>() }, { mp11::mp_size_t<0>() } }, ix_(0)
    {
    }

    template <class I, class... A>
    constexpr explicit variant_base_impl(I, A&&... a)
        : st_ { { mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)... }, { mp11::mp_size_t<0>() } }, ix_((I::value + 1) * 2)
    {
    }

    constexpr variant_storage<none, T...>& storage(unsigned i2) noexcept
    {
        return st_[i2];
    }

    constexpr variant_storage<none, T...> const& storage(unsigned i2) const noexcept
    {
        return st_[i2];
    }

    // requires: ix_ == 0
    template <class I, class... A>
    void _replace(I, A&&... a)
    {
        ::new (&storage(0)) variant_storage<none, T...>(mp11::mp_size_t<I::value + 1>(), std::forward<A>(a)...);

        static_assert((I::value + 1) * 2 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = (I::value + 1) * 2;
    }

    //[&]( auto I ){
    //    using U = mp_at_c<mp_list<none, T...>, I>;
    //    st1_.get( I ).~U();
    //}

    struct _destroy_L1
    {
        variant_base_impl* this_;
        unsigned i2_;

        template <class I>
        void operator()(I) const noexcept
        {
            using U = mp11::mp_at<mp11::mp_list<none, T...>, I>;
            this_->storage(i2_).get(I()).~U();
        }
    };

    void _destroy() noexcept
    {
        mp11::mp_with_index<1 + sizeof...(T)>(ix_ / 2, _destroy_L1 { this, static_cast<unsigned>(ix_ & 1) });
    }

    ~variant_base_impl() noexcept
    {
        _destroy();
    }

    constexpr std::size_t index() const noexcept
    {
        return ix_ / 2 - 1;
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I>& _get_impl(mp11::mp_size_t<I>) noexcept
    {
        assert(index() == I);

        std::size_t const J = I + 1;

        constexpr mp11::mp_size_t<J> j {};
        return storage(ix_ & 1).get(j);
    }

    template <std::size_t I>
    constexpr mp11::mp_at_c<std::variant<T...>, I> const& _get_impl(mp11::mp_size_t<I>) const noexcept
    {
        assert(index() == I);

        // std::size_t const J = I+1;
        // constexpr mp_size_t<J> j{};

        return storage(ix_ & 1).get(mp11::mp_size_t<I + 1>());
    }

    template <std::size_t I, class... A>
    void emplace(A&&... a)
    {
        std::size_t const J = I + 1;

        unsigned i2 = 1 - (ix_ & 1);

        storage(i2).emplace(mp11::mp_size_t<J>(), std::forward<A>(a)...);
        _destroy();

        static_assert(J * 2 + 1 <= (std::numeric_limits<index_type>::max)(), "");
        ix_ = static_cast<index_type>(J * 2 + i2);
    }

    static constexpr bool uses_double_storage() noexcept
    {
        return true;
    }
};

}  // namespace detail

// in_place_type_t

template <class T>
struct in_place_type_t
{
};

template <class T>
constexpr in_place_type_t<T> in_place_type {};

namespace detail
{

template <class T>
struct is_in_place_type : std::false_type
{
};
template <class T>
struct is_in_place_type<in_place_type_t<T>> : std::true_type
{
};

}  // namespace detail

// in_place_index_t

template <std::size_t I>
struct in_place_index_t
{
};

template <std::size_t I>
constexpr in_place_index_t<I> in_place_index {};

namespace detail
{

template <class T>
struct is_in_place_index : std::false_type
{
};
template <std::size_t I>
struct is_in_place_index<in_place_index_t<I>> : std::true_type
{
};

}  // namespace detail

// is_nothrow_swappable

namespace detail
{

namespace det2
{

using std::swap;

template <class T>
using is_swappable_impl = decltype(swap(std::declval<T&>(), std::declval<T&>()));

template <class T>
using is_nothrow_swappable_impl = typename std::enable_if<noexcept(swap(std::declval<T&>(), std::declval<T&>()))>::type;

}  // namespace det2

template <class T>
struct is_swappable : mp11::mp_valid<det2::is_swappable_impl, T>
{
};

template <class T>
struct is_nothrow_swappable : mp11::mp_valid<det2::is_nothrow_swappable_impl, T>
{
};

// variant_cc_base

template <bool CopyConstructible, bool TriviallyCopyConstructible, class... T>
struct variant_cc_base_impl;

template <class... T>
using variant_cc_base =
    variant_cc_base_impl<mp11::mp_all<std::is_copy_constructible<T>...>::value, mp11::mp_all<detail::is_trivially_copy_constructible<T>...>::value, T...>;

template <class... T>
struct variant_cc_base_impl<true, true, T...> : public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

    variant_cc_base_impl() = default;
    variant_cc_base_impl(variant_cc_base_impl const&) = default;
    variant_cc_base_impl(variant_cc_base_impl&&) = default;
    variant_cc_base_impl& operator=(variant_cc_base_impl const&) = default;
    variant_cc_base_impl& operator=(variant_cc_base_impl&&) = default;
};

template <bool B, class... T>
struct variant_cc_base_impl<false, B, T...> : public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

    variant_cc_base_impl() = default;
    variant_cc_base_impl(variant_cc_base_impl const&) = delete;
    variant_cc_base_impl(variant_cc_base_impl&&) = default;
    variant_cc_base_impl& operator=(variant_cc_base_impl const&) = default;
    variant_cc_base_impl& operator=(variant_cc_base_impl&&) = default;
};

template <class... T>
struct variant_cc_base_impl<true, false, T...> : public variant_base<T...>
{
    using variant_base = detail::variant_base<T...>;
    using variant_base::variant_base;

public:
    // constructors

    variant_cc_base_impl() = default;

    // copy constructor

private:
    struct L1
    {
        variant_base* this_;
        variant_base const& r;

        template <class I>
        void operator()(I i) const
        {
            this_->_replace(i, r._get_impl(i));
        }
    };

public:
    variant_cc_base_impl(variant_cc_base_impl const& r) noexcept(mp11::mp_all<std::is_nothrow_copy_constructible<T>...>::value) : variant_base()
    {
        mp11::mp_with_index<sizeof...(T)>(r.index(), L1 { this, r });
    }

    // move constructor

    variant_cc_base_impl(variant_cc_base_impl&&) = default;

    // assignment

    variant_cc_base_impl& operator=(variant_cc_base_impl const&) = default;
    variant_cc_base_impl& operator=(variant_cc_base_impl&&) = default;
};

// variant_ca_base

template <bool CopyAssignable, bool TriviallyCopyAssignable, class... T>
struct variant_ca_base_impl;

template <class... T>
using variant_ca_base = variant_ca_base_impl<
    mp11::mp_all<std::is_copy_constructible<T>..., std::is_copy_assignable<T>...>::value,
    mp11::mp_all<std::is_trivially_destructible<T>..., detail::is_trivially_copy_constructible<T>..., detail::is_trivially_copy_assignable<T>...>::value, T...>;

template <class... T>
struct variant_ca_base_impl<true, true, T...> : public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

    variant_ca_base_impl() = default;
    variant_ca_base_impl(variant_ca_base_impl const&) = default;
    variant_ca_base_impl(variant_ca_base_impl&&) = default;
    variant_ca_base_impl& operator=(variant_ca_base_impl const&) = default;
    variant_ca_base_impl& operator=(variant_ca_base_impl&&) = default;
};

template <bool B, class... T>
struct variant_ca_base_impl<false, B, T...> : public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

    variant_ca_base_impl() = default;
    variant_ca_base_impl(variant_ca_base_impl const&) = default;
    variant_ca_base_impl(variant_ca_base_impl&&) = default;
    variant_ca_base_impl& operator=(variant_ca_base_impl const&) = delete;
    variant_ca_base_impl& operator=(variant_ca_base_impl&&) = default;
};

template <class... T>
struct variant_ca_base_impl<true, false, T...> : public variant_cc_base<T...>
{
    using variant_base = detail::variant_cc_base<T...>;
    using variant_base::variant_base;

public:
    // constructors

    variant_ca_base_impl() = default;
    variant_ca_base_impl(variant_ca_base_impl const&) = default;
    variant_ca_base_impl(variant_ca_base_impl&&) = default;

    // copy assignment

private:
    struct L3
    {
        variant_base* this_;
        variant_base const& r;

        template <class I>
        void operator()(I i) const
        {
            this_->template emplace<I::value>(r._get_impl(i));
        }
    };

public:
    constexpr variant_ca_base_impl& operator=(variant_ca_base_impl const& r) noexcept(mp11::mp_all<std::is_nothrow_copy_constructible<T>...>::value)
    {
        mp11::mp_with_index<sizeof...(T)>(r.index(), L3 { this, r });
        return *this;
    }

    // move assignment

    variant_ca_base_impl& operator=(variant_ca_base_impl&&) = default;
};

// variant_mc_base

template <bool MoveConstructible, bool TriviallyMoveConstructible, class... T>
struct variant_mc_base_impl;

template <class... T>
using variant_mc_base =
    variant_mc_base_impl<mp11::mp_all<std::is_move_constructible<T>...>::value, mp11::mp_all<detail::is_trivially_move_constructible<T>...>::value, T...>;

template <class... T>
struct variant_mc_base_impl<true, true, T...> : public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

    variant_mc_base_impl() = default;
    variant_mc_base_impl(variant_mc_base_impl const&) = default;
    variant_mc_base_impl(variant_mc_base_impl&&) = default;
    variant_mc_base_impl& operator=(variant_mc_base_impl const&) = default;
    variant_mc_base_impl& operator=(variant_mc_base_impl&&) = default;
};

template <bool B, class... T>
struct variant_mc_base_impl<false, B, T...> : public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

    variant_mc_base_impl() = default;
    variant_mc_base_impl(variant_mc_base_impl const&) = default;
    variant_mc_base_impl(variant_mc_base_impl&&) = delete;
    variant_mc_base_impl& operator=(variant_mc_base_impl const&) = default;
    variant_mc_base_impl& operator=(variant_mc_base_impl&&) = default;
};

template <class... T>
struct variant_mc_base_impl<true, false, T...> : public variant_ca_base<T...>
{
    using variant_base = detail::variant_ca_base<T...>;
    using variant_base::variant_base;

public:
    // constructors

    variant_mc_base_impl() = default;
    variant_mc_base_impl(variant_mc_base_impl const&) = default;

    // move constructor

private:
    struct L2
    {
        variant_base* this_;
        variant_base& r;

        template <class I>
        void operator()(I i) const
        {
            this_->_replace(i, std::move(r._get_impl(i)));
        }
    };

public:
    variant_mc_base_impl(variant_mc_base_impl&& r) noexcept(mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value)
    {
        mp11::mp_with_index<sizeof...(T)>(r.index(), L2 { this, r });
    }

    // assignment

    variant_mc_base_impl& operator=(variant_mc_base_impl const&) = default;
    variant_mc_base_impl& operator=(variant_mc_base_impl&&) = default;
};

// variant_ma_base

template <bool MoveAssignable, bool TriviallyMoveAssignable, class... T>
struct variant_ma_base_impl;

template <class... T>
using variant_ma_base = variant_ma_base_impl<
    mp11::mp_all<std::is_move_constructible<T>..., std::is_move_assignable<T>...>::value,
    mp11::mp_all<std::is_trivially_destructible<T>..., detail::is_trivially_move_constructible<T>..., detail::is_trivially_move_assignable<T>...>::value, T...>;

template <class... T>
struct variant_ma_base_impl<true, true, T...> : public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

    variant_ma_base_impl() = default;
    variant_ma_base_impl(variant_ma_base_impl const&) = default;
    variant_ma_base_impl(variant_ma_base_impl&&) = default;
    variant_ma_base_impl& operator=(variant_ma_base_impl const&) = default;
    variant_ma_base_impl& operator=(variant_ma_base_impl&&) = default;
};

template <bool B, class... T>
struct variant_ma_base_impl<false, B, T...> : public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

    variant_ma_base_impl() = default;
    variant_ma_base_impl(variant_ma_base_impl const&) = default;
    variant_ma_base_impl(variant_ma_base_impl&&) = default;
    variant_ma_base_impl& operator=(variant_ma_base_impl const&) = default;
    variant_ma_base_impl& operator=(variant_ma_base_impl&&) = delete;
};

template <class... T>
struct variant_ma_base_impl<true, false, T...> : public variant_mc_base<T...>
{
    using variant_base = detail::variant_mc_base<T...>;
    using variant_base::variant_base;

public:
    // constructors

    variant_ma_base_impl() = default;
    variant_ma_base_impl(variant_ma_base_impl const&) = default;
    variant_ma_base_impl(variant_ma_base_impl&&) = default;

    // copy assignment

    variant_ma_base_impl& operator=(variant_ma_base_impl const&) = default;

    // move assignment

private:
    struct L4
    {
        variant_base* this_;
        variant_base& r;

        template <class I>
        void operator()(I i) const
        {
            this_->template emplace<I::value>(std::move(r._get_impl(i)));
        }
    };

public:
    variant_ma_base_impl& operator=(variant_ma_base_impl&& r) noexcept(mp11::mp_all<std::is_nothrow_move_constructible<T>...>::value)
    {
        mp11::mp_with_index<sizeof...(T)>(r.index(), L4 { this, r });
        return *this;
    }
};

}  // namespace detail

// relational operators

namespace detail
{

template <class... T>
struct eq_L
{
    std::variant<T...> const& v;
    std::variant<T...> const& w;

    template <class I>
    constexpr bool operator()(I i) const
    {
        return v._get_impl(i) == w._get_impl(i);
    }
};

}  // namespace detail

template <class... T>
constexpr bool operator==(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>(v.index(), detail::eq_L<T...> { v, w });
}

namespace detail
{

template <class... T>
struct ne_L
{
    std::variant<T...> const& v;
    std::variant<T...> const& w;

    template <class I>
    constexpr bool operator()(I i) const
    {
        return v._get_impl(i) != w._get_impl(i);
    }
};

}  // namespace detail

template <class... T>
constexpr bool operator!=(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return v.index() != w.index() || mp11::mp_with_index<sizeof...(T)>(v.index(), detail::ne_L<T...> { v, w });
}

namespace detail
{

template <class... T>
struct lt_L
{
    std::variant<T...> const& v;
    std::variant<T...> const& w;

    template <class I>
    constexpr bool operator()(I i) const
    {
        return v._get_impl(i) < w._get_impl(i);
    }
};

}  // namespace detail

template <class... T>
constexpr bool operator<(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return v.index() < w.index() || (v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>(v.index(), detail::lt_L<T...> { v, w }));
}

template <class... T>
constexpr bool operator>(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return w < v;
}

namespace detail
{

template <class... T>
struct le_L
{
    std::variant<T...> const& v;
    std::variant<T...> const& w;

    template <class I>
    constexpr bool operator()(I i) const
    {
        return v._get_impl(i) <= w._get_impl(i);
    }
};

}  // namespace detail

template <class... T>
constexpr bool operator<=(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return v.index() < w.index() || (v.index() == w.index() && mp11::mp_with_index<sizeof...(T)>(v.index(), detail::le_L<T...> { v, w }));
}

template <class... T>
constexpr bool operator>=(std::variant<T...> const& v, std::variant<T...> const& w)
{
    return w <= v;
}

// visitation
namespace detail
{

template <class T>
using remove_cv_ref_t = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template <class... T>
std::variant<T...> const& extract_variant_base_(std::variant<T...> const&);

template <class V>
using extract_variant_base = remove_cv_ref_t<decltype(extract_variant_base_(std::declval<V>()))>;

template <class V>
using variant_base_size = variant_size<extract_variant_base<V>>;

template <class T, class U>
struct copy_cv_ref
{
    using type = T;
};

template <class T, class U>
struct copy_cv_ref<T, U const>
{
    using type = T const;
};

template <class T, class U>
struct copy_cv_ref<T, U volatile>
{
    using type = T volatile;
};

template <class T, class U>
struct copy_cv_ref<T, U const volatile>
{
    using type = T const volatile;
};

template <class T, class U>
struct copy_cv_ref<T, U&>
{
    using type = typename copy_cv_ref<T, U>::type&;
};

template <class T, class U>
struct copy_cv_ref<T, U&&>
{
    using type = typename copy_cv_ref<T, U>::type&&;
};

template <class T, class U>
using copy_cv_ref_t = typename copy_cv_ref<T, U>::type;

template <class F>
struct Qret
{
    template <class... T>
    using fn = decltype(std::declval<F>()(std::declval<T>()...));
};

template <class L>
using front_if_same = mp11::mp_if<mp11::mp_apply<mp11::mp_same, L>, mp11::mp_front<L>>;

template <class V>
using apply_cv_ref = mp11::mp_product<copy_cv_ref_t, extract_variant_base<V>, mp11::mp_list<V>>;

struct deduced
{
};

template <class R, class F, class... V>
using Vret = mp11::mp_eval_if_not<std::is_same<R, deduced>, R, front_if_same, mp11::mp_product_q<Qret<F>, apply_cv_ref<V>...>>;

}  // namespace detail

template <class R = detail::deduced, class F>
constexpr auto visit(F&& f) -> detail::Vret<R, F>
{
    return std::forward<F>(f)();
}

namespace detail
{

template <class R, class F, class V1>
struct visit_L1
{
    F&& f;
    V1&& v1;

    template <class I>
    constexpr auto operator()(I) const -> Vret<R, F, V1>
    {
        return std::forward<F>(f)(unsafe_get<I::value>(std::forward<V1>(v1)));
    }
};

}  // namespace detail

template <class R = detail::deduced, class F, class V1>
constexpr auto visit(F&& f, V1&& v1) -> detail::Vret<R, F, V1>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>(v1.index(), detail::visit_L1<R, F, V1> { std::forward<F>(f), std::forward<V1>(v1) });
}

template <class R = detail::deduced, class F, class V1, class V2, class... V>
constexpr auto visit(F&& f, V1&& v1, V2&& v2, V&&... v) -> detail::Vret<R, F, V1, V2, V...>
{
    return mp11::mp_with_index<detail::variant_base_size<V1>>(v1.index(), [&](auto I) {
        auto f2 = [&](auto&&... a) { return std::forward<F>(f)(unsafe_get<I.value>(std::forward<V1>(v1)), std::forward<decltype(a)>(a)...); };
        return visit<R>(f2, std::forward<V2>(v2), std::forward<V>(v)...);
    });
}

// specialized algorithms
template <class... T, class E = typename std::enable_if<mp11::mp_all<std::is_move_constructible<T>..., detail::is_swappable<T>...>::value>::type>
void swap(std::variant<T...>& v, std::variant<T...>& w) noexcept(noexcept(v.swap(w)))
{
    v.swap(w);
}

// visit_by_index

namespace detail
{

template <class R, class V, class... F>
using Vret2 =
    mp11::mp_eval_if_not<std::is_same<R, deduced>, R, front_if_same, mp11::mp_transform<mp11::mp_invoke_q, mp11::mp_list<Qret<F>...>, apply_cv_ref<V>>>;

template <class R, class V, class... F>
struct visit_by_index_L
{
    V&& v;
    std::tuple<F&&...> tp;

    template <class I>
    constexpr detail::Vret2<R, V, F...> operator()(I) const
    {
        return std::get<I::value>(std::move(tp))(unsafe_get<I::value>(std::forward<V>(v)));
    }
};

}  // namespace detail

template <class R = detail::deduced, class V, class... F>
constexpr auto visit_by_index(V&& v, F&&... f) -> detail::Vret2<R, V, F...>
{
    static_assert(variant_size<V>::value == sizeof...(F), "Incorrect number of function objects");

    return mp11::mp_with_index<variant_size<V>::value>(v.index(),
                                                       detail::visit_by_index_L<R, V, F...> { std::forward<V>(v), std::tuple<F&&...>(std::forward<F>(f)...) });
}

// output streaming

namespace detail
{

template <class Ch, class Tr, class... T>
struct ostream_insert_L
{
    std::basic_ostream<Ch, Tr>& os;
    std::variant<T...> const& v;

    template <class I>
    std::basic_ostream<Ch, Tr>& operator()(I) const
    {
        return os << unsafe_get<I::value>(v);
    }
};

template <class Os, class T, class E = void>
struct is_output_streamable : std::false_type
{
};

template <class Os, class T>
struct is_output_streamable<Os, T, decltype(std::declval<Os&>() << std::declval<T const&>(), (void)0)> : std::true_type
{
};

}  // namespace detail

template <class Ch, class Tr>
std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& os, std::monostate const&)
{
    os << "monostate";
    return os;
}

template <class Ch, class Tr, class T1, class... T,
          class E = typename std::enable_if<mp11::mp_all<detail::is_output_streamable<std::basic_ostream<Ch, Tr>, T>...>::value>::type>
std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& os, std::variant<T1, T...> const& v)
{
    return mp11::mp_with_index<1 + sizeof...(T)>(v.index(), detail::ostream_insert_L<Ch, Tr, T1, T...> { os, v });
}

// hashing support

namespace detail
{

inline std::size_t hash_value_impl_(mp11::mp_true, std::size_t index, std::size_t value)
{
    unsigned long long hv = 0xCBF29CE484222325ull;
    unsigned long long const prime = 0x100000001B3ull;

    hv ^= index;
    hv *= prime;

    hv ^= value;
    hv *= prime;

    return static_cast<std::size_t>(hv);
}

inline std::size_t hash_value_impl_(mp11::mp_false, std::size_t index, std::size_t value)
{
    std::size_t hv = 0x811C9DC5;
    std::size_t const prime = 0x01000193;

    hv ^= index;
    hv *= prime;

    hv ^= value;
    hv *= prime;

    return hv;
}

inline std::size_t hash_value_impl(std::size_t index, std::size_t value)
{
    return hash_value_impl_(mp11::mp_bool<(SIZE_MAX > UINT32_MAX)>(), index, value);
}

template <template <class> class H, class V>
struct hash_value_L
{
    V const& v;

    template <class I>
    std::size_t operator()(I) const
    {
        auto const& t = unsafe_get<I::value>(v);

        std::size_t index = I::value;
        std::size_t value = H<remove_cv_ref_t<decltype(t)>>()(t);

        return hash_value_impl(index, value);
    }
};

template <class... T>
std::size_t hash_value_std(std::variant<T...> const& v)
{
    return mp11::mp_with_index<sizeof...(T)>(v.index(), detail::hash_value_L<std::hash, std::variant<T...>> { v });
}

}  // namespace detail

inline std::size_t hash_value(std::monostate const&)
{
    return 0xA7EE4757u;
}

template <class... T>
std::size_t hash_value(std::variant<T...> const& v)
{
    return mp11::mp_with_index<sizeof...(T)>(v.index(), detail::hash_value_L<::hash, std::variant<T...>> { v });
}

namespace detail
{

template <class T>
using is_hash_enabled = std::is_default_constructible<std::hash<typename std::remove_const<T>::type>>;

template <class V, bool E = mp11::mp_all_of<V, is_hash_enabled>::value>
struct std_hash_impl;

template <class V>
struct std_hash_impl<V, false>
{
    std_hash_impl() = delete;
    std_hash_impl(std_hash_impl const&) = delete;
    std_hash_impl& operator=(std_hash_impl const&) = delete;
};

template <class V>
struct std_hash_impl<V, true>
{
    std::size_t operator()(V const& v) const
    {
        return detail::hash_value_std(v);
    }
};

}  // namespace detail

}  // namespace variant2

// JSON support

namespace json
{

class value;

struct value_from_tag;

template <class T>
void value_from(T&& t, value& jv);

template <class T>
struct try_value_to_tag;

template <class T1, class T2>
struct result_for;

template <class T>
typename result_for<T, value>::type try_value_to(value const& jv);

template <class T>
typename result_for<T, value>::type result_from_errno(int e, std::source_location const* loc) noexcept;

template <class T>
struct is_null_like;

template <>
struct is_null_like<std::monostate> : std::true_type
{
};

}  // namespace json

namespace variant2
{

namespace detail
{

struct tag_invoke_L1
{
    json::value& v;

    template <class T>
    void operator()(T const& t) const
    {
        json::value_from(t, v);
    }
};

}  // namespace detail

template <class... T>
void tag_invoke(json::value_from_tag const&, json::value& v, std::variant<T...> const& w)
{
    visit(detail::tag_invoke_L1 { v }, w);
}

namespace detail
{

template <class V>
struct tag_invoke_L2
{
    json::value const& v;
    typename json::result_for<V, json::value>::type& r;

    template <class I>
    void operator()(I /*i*/) const
    {
        if (!r)
        {
            using Ti = mp11::mp_at_c<V, I::value>;
            auto r2 = json::try_value_to<Ti>(v);

            if (r2)
            {
                r.emplace(in_place_index_t<I::value> {}, std::move(*r2));
            }
        }
    }
};

}  // namespace detail

template <class... T>
typename json::result_for<std::variant<T...>, json::value>::type tag_invoke(json::try_value_to_tag<std::variant<T...>> const&, json::value const& v)
{
    static constexpr std::source_location loc = std::source_location::current();
    auto r = json::result_from_errno<std::variant<T...>>(EINVAL, &loc);

    mp11::mp_for_each<mp11::mp_iota_c<sizeof...(T)>>(detail::tag_invoke_L2<std::variant<T...>> { v, r });

    return r;
}

}  // namespace variant2

#endif  // #ifndef VARIANT2_VARIANT_HPP_INCLUDED
