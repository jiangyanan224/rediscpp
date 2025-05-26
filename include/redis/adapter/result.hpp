
/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_ADAPTER_RESULT_HPP
#define REDIS_ADAPTER_RESULT_HPP

#include <redis/resp3/type.hpp>
#include <redis/error.hpp>
#include <system/result.hpp>
#include <string>
#include <source_location>

namespace redis::adapter
{

/** @brief Stores any resp3 error
 *  @ingroup high-level-api
 */
struct error
{
    /// RESP3 error data type.
    resp3::type data_type = resp3::type::invalid;

    /// Diagnostic error message sent by Redis.
    std::string diagnostic;
};

/** @brief Compares two error objects for equality
 *  @relates error
 *
 *  @param a Left hand side error object.
 *  @param b Right hand side error object.
 */
inline bool operator==(error const& a, error const& b)
{
    return a.data_type == b.data_type && a.diagnostic == b.diagnostic;
}

/** @brief Compares two error objects for difference
 *  @relates error
 *
 *  @param a Left hand side error object.
 *  @param b Right hand side error object.
 */
inline bool operator!=(error const& a, error const& b)
{
    return !(a == b);
}

/** @brief Stores response to individual Redis commands
 *  @ingroup high-level-api
 */
template <class Value>
using result = system_::result<Value, error>;

inline void throw_exception_from_error(error const& e, std::source_location const&)
{
    std::error_code ec;
    switch (e.data_type)
    {
    case resp3::type::simple_error:
        ec = redis::error::resp3_simple_error;
        break;
    case resp3::type::blob_error:
        ec = redis::error::resp3_blob_error;
        break;
    case resp3::type::null:
        ec = redis::error::resp3_null;
        break;
    default:
        assert(false);
        // ASSERT_MSG(false, "Unexpected data type.");
    }

    throw std::system_error(ec, e.diagnostic);
}

}  // namespace redis::adapter

#endif  // REDIS_ADAPTER_RESULT_HPP