
#ifndef REDIS_MP_HPP
#define REDIS_MP_HPP

#include <tuple>
#include <type_traits>

namespace redis
{

// mp_list<T...>
template <class... T>
struct mp_list
{
};

// mp_size_t
template <std::size_t N>
using mp_size_t = std::integral_constant<std::size_t, N>;

// mp_bool
template <bool B>
using mp_bool = std::integral_constant<bool, B>;

using mp_true = mp_bool<true>;
using mp_false = mp_bool<false>;

namespace detail
{
// If
template <bool C, typename T, typename... E>
struct mp_if_c_impl
{
};

template <typename T, typename... E>
struct mp_if_c_impl<true, T, E...>
{
    using type = T;
};

template <typename T, typename E>
struct mp_if_c_impl<false, T, E>
{
    using type = E;
};
}  // namespace detail

template <bool C, typename T, typename... E>
using mp_if_c = typename detail::mp_if_c_impl<C, T, E...>::type;

template <typename C, typename T, typename... E>
using mp_if = typename detail::mp_if_c_impl<static_cast<bool>(C::value), T, E...>::type;

template <auto A>
using mp_value = std::integral_constant<decltype(A), A>;

// mp_count<L, V>
namespace detail
{

constexpr std::size_t cx_plus()
{
    return 0;
}

template <class T1, class... T>
constexpr std::size_t cx_plus(T1 t1, T... t)
{
    return static_cast<std::size_t>(t1) + cx_plus(t...);
}

template <class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10, class... T>
constexpr std::size_t cx_plus(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8, T9 t9, T10 t10, T... t)
{
    return static_cast<std::size_t>(t1 + t2 + t3 + t4 + t5 + t6 + t7 + t8 + t9 + t10) + cx_plus(t...);
}

template <class L, class V>
struct mp_count_impl;

template <class V, class... T>
constexpr std::size_t cx_count()
{
    constexpr bool a[] = { false, std::is_same<T, V>::value... };

    std::size_t r = 0;

    for (std::size_t i = 0; i < sizeof...(T); ++i)
    {
        r += a[i + 1];
    }

    return r;
}

template <template <class...> class L, class... T, class V>
struct mp_count_impl<L<T...>, V>
{
    using type = mp_size_t<cx_count<V, T...>()>;
};

}  // namespace detail

template <class L, class V>
using mp_count = typename detail::mp_count_impl<L, V>::type;

// mp_same<T...>
namespace detail
{

template <class... T>
struct mp_same_impl;

template <>
struct mp_same_impl<>
{
    using type = mp_true;
};

template <class T1, class... T>
struct mp_same_impl<T1, T...>
{
    using type = mp_bool<mp_count<mp_list<T...>, T1>::value == sizeof...(T)>;
};

}  // namespace detail

template <class... T>
using mp_same = typename detail::mp_same_impl<T...>::type;

// mp_size<L>
namespace detail
{

template <class L>
struct mp_size_impl
{
    // An error "no type named 'type'" here means that the argument to mp_size is not a list
};

template <template <class...> class L, class... T>
struct mp_size_impl<L<T...>>
{
    using type = mp_size_t<sizeof...(T)>;
};

template <template <auto...> class L, auto... A>
struct mp_size_impl<L<A...>>
{
    using type = mp_size_t<sizeof...(A)>;
};

}  // namespace detail

template <class L>
using mp_size = typename detail::mp_size_impl<L>::type;

// mp_transform<F, L...>
namespace detail
{

template <template <class...> class F, class... L>
struct mp_transform_impl
{
};

template <template <class...> class F, template <class...> class L, class... T>
struct mp_transform_impl<F, L<T...>>
{
    using type = L<F<T>...>;
};

template <template <class...> class F, template <class...> class L1, class... T1, template <class...> class L2, class... T2>
struct mp_transform_impl<F, L1<T1...>, L2<T2...>>
{
    using type = L1<F<T1, T2>...>;
};

template <template <class...> class F, template <class...> class L1, class... T1, template <class...> class L2, class... T2, template <class...> class L3,
          class... T3>
struct mp_transform_impl<F, L1<T1...>, L2<T2...>, L3<T3...>>
{
    using type = L1<F<T1, T2, T3>...>;
};

template <template <class...> class F, template <auto...> class L, auto... A>
struct mp_transform_impl<F, L<A...>>
{
    using type = L<F<mp_value<A>>::value...>;
};

template <template <class...> class F, template <auto...> class L1, auto... A1, template <auto...> class L2, auto... A2>
struct mp_transform_impl<F, L1<A1...>, L2<A2...>>
{
    using type = L1<F<mp_value<A1>, mp_value<A2>>::value...>;
};

template <template <class...> class F, template <auto...> class L1, auto... A1, template <auto...> class L2, auto... A2, template <auto...> class L3,
          auto... A3>
struct mp_transform_impl<F, L1<A1...>, L2<A2...>, L3<A3...>>
{
    using type = L1<F<mp_value<A1>, mp_value<A2>, mp_value<A3>>::value...>;
};

struct list_size_mismatch
{
};

}  // namespace detail

template <template <class...> class F, class... L>
using mp_transform = typename mp_if<mp_same<mp_size<L>...>, detail::mp_transform_impl<F, L...>, detail::list_size_mismatch>::type;

// mp_defer
namespace detail
{

template <template <class...> class F, class... T>
struct mp_defer_impl
{
    using type = F<T...>;
};

struct mp_no_type
{
};

}  // namespace detail

namespace detail
{

template <template <class...> class F, class... T>
struct mp_valid_impl
{
    template <template <class...> class G, class = G<T...>>
    static mp_true check(int);
    template <template <class...> class>
    static mp_false check(...);

    using type = decltype(check<F>(0));
};

}  // namespace detail

template <template <class...> class F, class... T>
using mp_valid = typename detail::mp_valid_impl<F, T...>::type;

template <template <class...> class F, class... T>
using mp_defer = mp_if<mp_valid<F, T...>, detail::mp_defer_impl<F, T...>, detail::mp_no_type>;

// mp_rename<L, B>
namespace detail
{

template <class L, template <class...> class B>
struct mp_rename_impl
{
    // An error "no type named 'type'" here means that the first argument to mp_rename is not a list
};

template <template <class...> class L, class... T, template <class...> class B>
struct mp_rename_impl<L<T...>, B> : mp_defer<B, T...>
{
};

template <template <auto...> class L, auto... A, template <class...> class B>
struct mp_rename_impl<L<A...>, B> : mp_defer<B, mp_value<A>...>
{
};

}  // namespace detail

template <class L, template <class...> class B>
using mp_rename = typename detail::mp_rename_impl<L, B>::type;

// // 定义 transform 基础模板
// template <template <typename> class Func, typename... Types>
// struct mp_transform_t;

// // 偏特化展开模板参数
// template <template <typename> class Func, typename T, typename... Rest>
// struct mp_transform_t<Func, T, Rest...>
// {
//     using type = std::tuple<typename Func<T>::type, typename Func<Rest>::type...>;
// };

// // 辅助别名模板
// template <template <typename> class Func, typename... Types>
// using mp_transform = typename mp_transform_t<Func, Types...>::type;

// // 自定义 mp_rename 的实现
// template <typename T, template <typename...> class TargetTemplate>
// struct mp_rename_t;

// template <template <typename...> class SourceTemplate, typename... Args, template <typename...> class TargetTemplate>
// struct mp_rename_t<SourceTemplate<Args...>, TargetTemplate>
// {
//     using type = TargetTemplate<Args...>;
// };

// // 简化用法
// template <typename T, template <typename...> class TargetTemplate>
// using mp_rename = typename mp_rename_t<T, TargetTemplate>::type;

}  // namespace redis

#endif  // REDIS_IGNORE_HPP
