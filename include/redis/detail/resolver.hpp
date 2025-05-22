/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_RESOLVER_HPP
#define REDIS_RESOLVER_HPP

#include <redis/config.hpp>
#include <redis/detail/helper.hpp>
#include <redis/error.hpp>
#include <asio/compose.hpp>
#include <asio/coroutine.hpp>
#include <asio/experimental/parallel_group.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <string>
#include <chrono>
#include <system_error>

namespace redis::detail
{

template <class Resolver>
struct resolve_op
{
    Resolver* resv_ = nullptr;
    asio::coroutine coro {};

    template <class Self>
    void operator()(Self& self, std::array<std::size_t, 2> order = {}, std::error_code ec1 = {}, asio::ip::tcp::resolver::results_type res = {},
                    std::error_code ec2 = {})
    {
        ASIO_CORO_REENTER(coro)
        {
            resv_->timer_.expires_after(resv_->timeout_);

            ASIO_CORO_YIELD
            asio::experimental::make_parallel_group([this](auto token) { return resv_->resv_.async_resolve(resv_->addr_.host, resv_->addr_.port, token); },
                                                    [this](auto token) { return resv_->timer_.async_wait(token); })
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
                // Resolver completed first.
                resv_->results_ = res;
                self.complete(ec1);
            }
            break;

            case 1:
            {
                if (ec2)
                {
                    // Timer completed first with error, perhaps a
                    // cancellation going on.
                    self.complete(ec2);
                }
                else
                {
                    // Timer completed first without an error, this is a
                    // resolve timeout.
                    self.complete(error::resolve_timeout);
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
class resolver
{
public:
    using timer_type = asio::basic_waitable_timer<std::chrono::steady_clock, asio::wait_traits<std::chrono::steady_clock>, Executor>;

    resolver(Executor ex) : resv_ { ex }, timer_ { ex }
    {
    }

    template <class CompletionToken>
    auto async_resolve(CompletionToken&& token)
    {
        return asio::async_compose<CompletionToken, void(std::error_code)>(resolve_op<resolver> { this }, token, resv_);
    }

    std::size_t cancel(operation op)
    {
        switch (op)
        {
        case operation::resolve:
        case operation::all:
            resv_.cancel();
            timer_.cancel();
            break;
        default: /* ignore */;
        }

        return 0;
    }

    auto const& results() const noexcept
    {
        return results_;
    }

    void set_config(config const& cfg)
    {
        addr_ = cfg.addr;
        timeout_ = cfg.resolve_timeout;
    }

private:
    using resolver_type = asio::ip::basic_resolver<asio::ip::tcp, Executor>;
    template <class>
    friend struct resolve_op;

    resolver_type resv_;
    timer_type timer_;
    address addr_;
    std::chrono::steady_clock::duration timeout_;
    asio::ip::tcp::resolver::results_type results_;
};

}  // namespace redis::detail

#endif  // REDIS_RESOLVER_HPP
