/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_SSL_CONNECTOR_HPP
#define REDIS_SSL_CONNECTOR_HPP

#include <redis/detail/helper.hpp>
#include <redis/error.hpp>
#include <asio/compose.hpp>
#include <asio/connect.hpp>
#include <asio/coroutine.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/ssl.hpp>
#include <string>
#include <chrono>
#include <system_error>

namespace redis::detail
{

template <class Handshaker, class Stream>
struct handshake_op
{
    Handshaker* hsher_ = nullptr;
    Stream* stream_ = nullptr;
    asio::coroutine coro {};

    template <class Self>
    void operator()(Self& self, std::array<std::size_t, 2> const& order = {}, std::error_code const& ec1 = {}, std::error_code const& ec2 = {})
    {
        ASIO_CORO_REENTER(coro)
        {
            hsher_->timer_.expires_after(hsher_->timeout_);

            ASIO_CORO_YIELD
            asio::experimental::make_parallel_group([this](auto token) { return stream_->async_handshake(asio::ssl::stream_base::client, token); },
                                                    [this](auto token) { return hsher_->timer_.async_wait(token); })
                .async_wait(asio::experimental::wait_for_one(), std::move(self));

            if (is_cancelled(self))
            {
                self.complete(asio::error::operation_aborted);
                return;
            }

            switch (order[0])
            {
            case 0:
            {
                self.complete(ec1);
            }
            break;
            case 1:
            {
                if (ec2)
                {
                    self.complete(ec2);
                }
                else
                {
                    self.complete(error::ssl_handshake_timeout);
                }
            }
            break;

            default:
                assert(false);
            }
        }
    }
};

template <class Executor>
class handshaker
{
public:
    using timer_type = asio::basic_waitable_timer<std::chrono::steady_clock, asio::wait_traits<std::chrono::steady_clock>, Executor>;

    handshaker(Executor ex) : timer_ { ex }
    {
    }

    template <class Stream, class CompletionToken>
    auto async_handshake(Stream& stream, CompletionToken&& token)
    {
        return asio::async_compose<CompletionToken, void(std::error_code)>(handshake_op<handshaker, Stream> { this, &stream }, token, timer_);
    }

    std::size_t cancel(operation op)
    {
        switch (op)
        {
        case operation::ssl_handshake:
        case operation::all:
            timer_.cancel();
            break;
        default: /* ignore */;
        }

        return 0;
    }

    constexpr bool is_dummy() const noexcept
    {
        return false;
    }

    void set_config(config const& cfg)
    {
        timeout_ = cfg.ssl_handshake_timeout;
    }

private:
    template <class, class>
    friend struct handshake_op;

    timer_type timer_;
    std::chrono::steady_clock::duration timeout_;
};

}  // namespace redis::detail

#endif  // REDIS_SSL_CONNECTOR_HPP
