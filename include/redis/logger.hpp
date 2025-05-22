/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_LOGGER_HPP
#define REDIS_LOGGER_HPP

#include <redis/response.hpp>
#include <asio/ip/tcp.hpp>
#include <string>
#include <system_error>
#include <iostream>

namespace redis
{

/** @brief Logger class
 *  @ingroup high-level-api
 *
 *  The class can be passed to the connection objects to log to `std::clog`
 */
class logger
{
public:
    /** @brief Syslog-like log levels
     *  @ingroup high-level-api
     */
    enum class level
    {  /// Emergency
        emerg,

        /// Alert
        alert,

        /// Critical
        crit,

        /// Error
        err,

        /// Warning
        warning,

        /// Notice
        notice,

        /// Info
        info,

        /// Debug
        debug
    };

    /** @brief Constructor
     *  @ingroup high-level-api
     *
     *  @param l Log level.
     */
    logger(level l = level::info) : level_ { l }
    {
    }

    /** @brief Called when the resolve operation completes.
     *  @ingroup high-level-api
     *
     *  @param ec Error returned by the resolve operation.
     *  @param res Resolve results.
     */
    void on_resolve(std::error_code const& ec, asio::ip::tcp::resolver::results_type const& res)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        std::clog << "Resolve results: ";

        if (ec)
        {
            std::clog << ec.message() << std::endl;
        }
        else
        {
            auto begin = std::cbegin(res);
            auto end = std::cend(res);

            if (begin == end)
                return;

            std::clog << begin->endpoint();
            for (auto iter = std::next(begin); iter != end; ++iter)
                std::clog << ", " << iter->endpoint();
        }

        std::clog << std::endl;
    };

    /** @brief Called when the connect operation completes.
     *  @ingroup high-level-api
     *
     *  @param ec Error returned by the connect operation.
     *  @param ep Endpoint to which the connection connected.
     */
    void on_connect(std::error_code const& ec, asio::ip::tcp::endpoint const& ep)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        std::clog << "Connected to endpoint: ";

        if (ec)
            std::clog << ec.message() << std::endl;
        else
            std::clog << ep;

        std::clog << std::endl;
    };

    /** @brief Called when the ssl handshake operation completes.
     *  @ingroup high-level-api
     *
     *  @param ec Error returned by the handshake operation.
     */
    void on_ssl_handshake(std::error_code const& ec)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        std::clog << "SSL handshake: " << ec.message() << std::endl;
    };

    /** @brief Called when the connection is lost.
     *  @ingroup high-level-api
     *
     *  @param ec Error returned when the connection is lost.
     */
    void on_connection_lost(std::error_code const& ec)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        if (ec)
            std::clog << "Connection lost: " << ec.message();
        else
            std::clog << "Connection lost.";

        std::clog << std::endl;
    };

    /** @brief Called when the write operation completes.
     *  @ingroup high-level-api
     *
     *  @param ec Error code returned by the write operation.
     *  @param payload The payload written to the socket.
     */
    void on_write(std::error_code const& ec, std::string const& payload)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        if (ec)
            std::clog << "Write: " << ec.message();
        else
            std::clog << "Bytes written: " << std::size(payload);

        std::clog << std::endl;
    };

    /** @brief Called when the `HELLO` request completes.
     *  @ingroup high-level-api
     *
     *  @param ec Error code returned by the async_exec operation.
     *  @param resp Response sent by the Redis server.
     */
    void on_hello(std::error_code const& ec, generic_response const& resp)
    {
        if (level_ < level::info)
            return;

        write_prefix();

        if (ec)
        {
            std::clog << "Hello: " << ec.message();
            if (resp.has_error())
                std::clog << " (" << resp.error().diagnostic << ")";
        }
        else
        {
            std::clog << "Hello: Success";
        }

        std::clog << std::endl;
    };

    /** @brief Sets a prefix to every log message
     *  @ingroup high-level-api
     *
     *  @param prefix The prefix.
     */
    void set_prefix(std::string_view prefix)
    {
        prefix_ = prefix;
    }

private:
    void write_prefix()
    {
        if (!std::empty(prefix_))
            std::clog << prefix_;
    };

    level level_;
    std::string_view prefix_;
};

}  // namespace redis

#endif  // REDIS_LOGGER_HPP
