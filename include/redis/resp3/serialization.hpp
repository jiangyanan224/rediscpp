/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_RESP3_SERIALIZATION_HPP
#define REDIS_RESP3_SERIALIZATION_HPP

#include <redis/resp3/type.hpp>
#include <redis/resp3/parser.hpp>

#include <system_error>
#include <string>
#include <tuple>

// NOTE: Consider detecting tuples in the type in the parameter pack
// to calculate the header size correctly.

namespace redis::resp3
{

/** @brief Adds a bulk to the request.
 *  @relates redis::request
 *
 *  This function is useful in serialization of your own data
 *  structures in a request. For example
 *
 *  @code
 *  void boost_redis_to_bulk(std::string& payload, mystruct const& obj)
 *  {
 *     auto const str = // Convert obj to a string.
 *     boost_redis_to_bulk(payload, str);
 *  }
 *  @endcode
 *
 *  @param payload Storage on which data will be copied into.
 *  @param data Data that will be serialized and stored in `payload`.
 *
 *  See more in @ref serialization.
 */

inline void boost_redis_to_bulk(std::string& payload, std::string_view data)
{
    auto const str = std::to_string(data.size());

    payload += to_code(type::blob_string);
    payload.append(std::cbegin(str), std::cend(str));
    payload += parser::sep;
    payload.append(std::cbegin(data), std::cend(data));
    payload += parser::sep;
};

template <class T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
void boost_redis_to_bulk(std::string& payload, T n)
{
    auto const s = std::to_string(n);
    redis::resp3::boost_redis_to_bulk(payload, std::string_view { s });
}

template <class T>
struct add_bulk_impl
{
    static void add(std::string& payload, T const& from)
    {
        using namespace redis::resp3;
        boost_redis_to_bulk(payload, from);
    }
};

template <class... Ts>
struct add_bulk_impl<std::tuple<Ts...>>
{
    static void add(std::string& payload, std::tuple<Ts...> const& t)
    {
        auto f = [&](auto const&... vs) {
            using namespace redis::resp3;
            (boost_redis_to_bulk(payload, vs), ...);
        };

        std::apply(f, t);
    }
};

template <class U, class V>
struct add_bulk_impl<std::pair<U, V>>
{
    static void add(std::string& payload, std::pair<U, V> const& from)
    {
        using namespace redis::resp3;
        boost_redis_to_bulk(payload, from.first);
        boost_redis_to_bulk(payload, from.second);
    }
};

inline void add_header(std::string& payload, type t, std::size_t size)
{
    auto const str = std::to_string(size);

    payload += to_code(t);
    payload.append(std::cbegin(str), std::cend(str));
    payload += parser::sep;
};

template <class T>
void add_bulk(std::string& payload, T const& data)
{
    add_bulk_impl<T>::add(payload, data);
}

template <class>
struct bulk_counter;

template <class>
struct bulk_counter
{
    static constexpr auto size = 1U;
};

template <class T, class U>
struct bulk_counter<std::pair<T, U>>
{
    static constexpr auto size = 2U;
};

inline void add_blob(std::string& payload, std::string_view blob)
{
    payload.append(std::cbegin(blob), std::cend(blob));
    payload += parser::sep;
};

inline void add_separator(std::string& payload)
{
    payload += parser::sep;
};

namespace detail
{

template <class Adapter>
void deserialize(std::string_view const& data, Adapter adapter, std::error_code& ec)
{
    parser parser;
    while (!parser.done())
    {
        auto const res = parser.consume(data, ec);
        if (ec)
            return;

        assert(res.has_value());

        adapter(res.value(), ec);
        if (ec)
            return;
    }

    assert(parser.get_consumed() == std::size(data));
}

template <class Adapter>
void deserialize(std::string_view const& data, Adapter adapter)
{
    std::error_code ec;
    deserialize(data, adapter, ec);

    if (ec)
        throw std::system_error { ec };
}

}  // namespace detail

}  // namespace redis::resp3

#endif  // REDIS_RESP3_SERIALIZATION_HPP
