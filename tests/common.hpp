#pragma once

#include <system/error_code.hpp>
#include <asio/redirect_error.hpp>
#include <asio/awaitable.hpp>
#include <asio/use_awaitable.hpp>
#include <redis/connection.hpp>
#include <redis/operation.hpp>
#include <memory>

#ifdef BOOST_ASIO_HAS_CO_AWAIT
inline
auto redir(boost::system::error_code& ec)
   { return boost::asio::redirect_error(boost::asio::use_awaitable, ec); }
auto start(boost::asio::awaitable<void> op) -> int;
#endif // BOOST_ASIO_HAS_CO_AWAIT

void
run(
   std::shared_ptr<boost::redis::connection> conn,
   boost::redis::config cfg = {},
   boost::system::error_code ec = boost::asio::error::operation_aborted,
   boost::redis::operation op = boost::redis::operation::receive,
   boost::redis::logger::level l = boost::redis::logger::level::info);

