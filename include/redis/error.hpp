/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_ERROR_HPP
#define REDIS_ERROR_HPP

#include <system_error>

namespace redis
{

/** \brief Generic errors.
 *  \ingroup high-level-api
 */
enum class error
{
    /// Invalid RESP3 type.
    invalid_data_type = 1,

    /// Can't parse the string as a number.
    not_a_number,

    /// The maximum depth of a nested response was exceeded.
    exceeeds_max_nested_depth,

    /// Got non boolean value.
    unexpected_bool_value,

    /// Expected field value is empty.
    empty_field,

    /// Expects a simple RESP3 type but got an aggregate.
    expects_resp3_simple_type,

    /// Expects aggregate.
    expects_resp3_aggregate,

    /// Expects a map but got other aggregate.
    expects_resp3_map,

    /// Expects a set aggregate but got something else.
    expects_resp3_set,

    /// Nested response not supported.
    nested_aggregate_not_supported,

    /// Got RESP3 simple error.
    resp3_simple_error,

    /// Got RESP3 blob_error.
    resp3_blob_error,

    /// Aggregate container has incompatible size.
    incompatible_size,

    /// Not a double
    not_a_double,

    /// Got RESP3 null.
    resp3_null,

    /// There is no stablished connection.
    not_connected,

    /// Resolve timeout
    resolve_timeout,

    /// Connect timeout
    connect_timeout,

    /// Connect timeout
    pong_timeout,

    /// SSL handshake timeout
    ssl_handshake_timeout,
};

namespace detail
{

struct error_category_impl : std::error_category
{
    virtual ~error_category_impl() = default;

    auto name() const noexcept -> char const* override
    {
        return "redis";
    }

    auto message(int ev) const -> std::string override
    {
        switch (static_cast<error>(ev))
        {
        case error::invalid_data_type:
            return "Invalid resp3 type.";
        case error::not_a_number:
            return "Can't convert string to number (maybe forgot to upgrade to RESP3?).";
        case error::exceeeds_max_nested_depth:
            return "Exceeds the maximum number of nested responses.";
        case error::unexpected_bool_value:
            return "Unexpected bool value.";
        case error::empty_field:
            return "Expected field value is empty.";
        case error::expects_resp3_simple_type:
            return "Expects a resp3 simple type.";
        case error::expects_resp3_aggregate:
            return "Expects resp3 aggregate.";
        case error::expects_resp3_map:
            return "Expects resp3 map.";
        case error::expects_resp3_set:
            return "Expects resp3 set.";
        case error::nested_aggregate_not_supported:
            return "Nested aggregate not_supported.";
        case error::resp3_simple_error:
            return "Got RESP3 simple-error.";
        case error::resp3_blob_error:
            return "Got RESP3 blob-error.";
        case error::incompatible_size:
            return "Aggregate container has incompatible size.";
        case error::not_a_double:
            return "Not a double.";
        case error::resp3_null:
            return "Got RESP3 null.";
        case error::not_connected:
            return "Not connected.";
        case error::resolve_timeout:
            return "Resolve timeout.";
        case error::connect_timeout:
            return "Connect timeout.";
        case error::pong_timeout:
            return "Pong timeout.";
        default:
            assert(false);
            return "Redis error.";
        }
    }
};

auto category() -> std::error_category const&
{
    static error_category_impl instance;
    return instance;
}

}  // namespace detail

/** \internal
 *  \brief Creates a error_code object from an error.
 *  \param e Error code.
 *  \ingroup any
 */
inline auto make_error_code(error e) -> std::error_code
{
    return std::error_code { static_cast<int>(e), detail::category() };
};

}  // namespace redis

namespace std
{

template <>
struct is_error_code_enum<::redis::error> : std::true_type
{
};

}  // namespace std

#endif  // REDIS_ERROR_HPP
