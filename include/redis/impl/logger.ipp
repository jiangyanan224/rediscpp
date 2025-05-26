/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#include <redis/logger.hpp>
#include <iostream>
#include <iterator>

namespace redis
{

void logger::write_prefix()
{
    if (!std::empty(prefix_))
        std::clog << prefix_;
}

void logger::on_resolve(std::error_code const& ec, asio::ip::tcp::resolver::results_type const& res)
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
}

void logger::on_connect(std::error_code const& ec, asio::ip::tcp::endpoint const& ep)
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
}

void logger::on_ssl_handshake(std::error_code const& ec)
{
    if (level_ < level::info)
        return;

    write_prefix();

    std::clog << "SSL handshake: " << ec.message() << std::endl;
}

void logger::on_connection_lost(std::error_code const& ec)
{
    if (level_ < level::info)
        return;

    write_prefix();

    if (ec)
        std::clog << "Connection lost: " << ec.message();
    else
        std::clog << "Connection lost.";

    std::clog << std::endl;
}

void logger::on_write(std::error_code const& ec, std::string const& payload)
{
    if (level_ < level::info)
        return;

    write_prefix();

    if (ec)
        std::clog << "Write: " << ec.message();
    else
        std::clog << "Bytes written: " << std::size(payload);

    std::clog << std::endl;
}

void logger::on_hello(std::error_code const& ec, generic_response const& resp)
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
}

}  // namespace redis
