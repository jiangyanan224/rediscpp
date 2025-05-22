/* Copyright (c) 2018-2022 Marcelo Zimbres Silva (mzimbres@gmail.com)
 *
 * Distributed under the Redis Software License, Version 1.0. (See
 * accompanying file LICENSE.txt)
 */

#ifndef REDIS_RESP3_PARSER_HPP
#define REDIS_RESP3_PARSER_HPP

#include <redis/resp3/node.hpp>
#include <system_error>
#include <array>
#include <limits>
#include <string_view>
#include <cstdint>
#include <optional>

namespace redis::resp3
{

using int_type = std::uint64_t;

inline void to_int(int_type& i, std::string_view sv, std::error_code& ec)
{
    auto const res = std::from_chars(sv.data(), sv.data() + std::size(sv), i);
    if (res.ec != std::errc())
        ec = error::not_a_number;
}

class parser
{
public:
    using node_type = basic_node<std::string_view>;
    using result = std::optional<node_type>;

    static constexpr std::size_t max_embedded_depth = 5;
    static constexpr std::string_view sep = "\r\n";

private:
    // The current depth. Simple data types will have depth 0, whereas
    // the elements of aggregates will have depth 1. Embedded types
    // will have increasing depth.
    std::size_t depth_ = 0;

    // The parser supports up to 5 levels of nested structures. The
    // first element in the sizes stack is a sentinel and must be
    // different from 1.
    std::array<std::size_t, max_embedded_depth + 1> sizes_ = { { 1 } };

    // Contains the length expected in the next bulk read.
    int_type bulk_length_ = (std::numeric_limits<unsigned long>::max)();

    // The type of the next bulk. Contains type::invalid if no bulk is
    // expected.
    type bulk_ = type::invalid;

    // The number of bytes consumed from the buffer.
    std::size_t consumed_ = 0;

    // Returns the number of bytes that have been consumed.
    auto consume_impl(type t, std::string_view elem, std::error_code& ec) -> node_type
    {
        assert(!bulk_expected());

        node_type ret;
        switch (t)
        {
        case type::streamed_string_part:
        {
            to_int(bulk_length_, elem, ec);
            if (ec)
                return {};

            if (bulk_length_ == 0)
            {
                ret = { type::streamed_string_part, 1, depth_, {} };
                sizes_[depth_] = 1;  // We are done.
                bulk_ = type::invalid;
                commit_elem();
            }
            else
            {
                bulk_ = type::streamed_string_part;
            }
        }
        break;
        case type::blob_error:
        case type::verbatim_string:
        case type::blob_string:
        {
            if (elem.at(0) == '?')
            {
                // NOTE: This can only be triggered with blob_string.
                // Trick: A streamed string is read as an aggregate of
                // infinite length. When the streaming is done the server
                // is supposed to send a part with length 0.
                sizes_[++depth_] = (std::numeric_limits<std::size_t>::max)();
                ret = { type::streamed_string, 0, depth_, {} };
            }
            else
            {
                to_int(bulk_length_, elem, ec);
                if (ec)
                    return {};

                bulk_ = t;
            }
        }
        break;
        case type::boolean:
        {
            if (std::empty(elem))
            {
                ec = error::empty_field;
                return {};
            }

            if (elem.at(0) != 'f' && elem.at(0) != 't')
            {
                ec = error::unexpected_bool_value;
                return {};
            }

            ret = { t, 1, depth_, elem };
            commit_elem();
        }
        break;
        case type::doublean:
        case type::big_number:
        case type::number:
        {
            if (std::empty(elem))
            {
                ec = error::empty_field;
                return {};
            }
        }
            [[fallthrough]];
        case type::simple_error:
        case type::simple_string:
        case type::null:
        {
            ret = { t, 1, depth_, elem };
            commit_elem();
        }
        break;
        case type::push:
        case type::set:
        case type::array:
        case type::attribute:
        case type::map:
        {
            int_type l = -1;
            to_int(l, elem, ec);
            if (ec)
                return {};

            ret = { t, l, depth_, {} };
            if (l == 0)
            {
                commit_elem();
            }
            else
            {
                if (depth_ == max_embedded_depth)
                {
                    ec = error::exceeeds_max_nested_depth;
                    return {};
                }

                ++depth_;

                sizes_[depth_] = l * element_multiplicity(t);
            }
        }
        break;
        default:
        {
            ec = error::invalid_data_type;
            return {};
        }
        }

        return ret;
    };

    void commit_elem() noexcept
    {
        --sizes_[depth_];
        while (sizes_[depth_] == 0)
        {
            --depth_;
            --sizes_[depth_];
        }
    };

    // The bulk type expected in the next read. If none is expected
    // returns type::invalid.
    [[nodiscard]]
    auto bulk_expected() const noexcept -> bool
    {
        return bulk_ != type::invalid;
    }

public:
    parser()
    {
        sizes_[0] = 2;  // The sentinel must be more than 1.
    };

    // Returns true when the parser is done with the current message.
    [[nodiscard]]
    auto done() const noexcept -> bool
    {
        return depth_ == 0 && bulk_ == type::invalid && consumed_ != 0;
    };

    auto get_suggested_buffer_growth(std::size_t hint) const noexcept -> std::size_t
    {
        if (!bulk_expected())
            return hint;

        if (hint < bulk_length_ + 2)
            return bulk_length_ + 2;

        return hint;
    };

    auto get_consumed() const noexcept -> std::size_t
    {
        return consumed_;
    };

    auto consume(std::string_view view, std::error_code& ec) noexcept -> result
    {
        switch (bulk_)
        {
        case type::invalid:
        {
            auto const pos = view.find(sep, consumed_);
            if (pos == std::string::npos)
                return {};  // Needs more data to proceeed.

            auto const t = to_type(view.at(consumed_));
            auto const content = view.substr(consumed_ + 1, pos - 1 - consumed_);
            auto const ret = consume_impl(t, content, ec);
            if (ec)
                return {};

            consumed_ = pos + 2;
            if (!bulk_expected())
                return ret;
        }
            [[fallthrough]];

        default:  // Handles bulk.
        {
            auto const span = bulk_length_ + 2;
            if ((std::size(view) - consumed_) < span)
                return {};  // Needs more data to proceeed.

            auto const bulk_view = view.substr(consumed_, bulk_length_);
            node_type const ret = { bulk_, 1, depth_, bulk_view };
            bulk_ = type::invalid;
            commit_elem();

            consumed_ += span;
            return ret;
        }
        }
    };
};

template <class Adapter>
bool parse(resp3::parser& p, std::string_view const& msg, Adapter& adapter, std::error_code& ec)
{
    while (!p.done())
    {
        auto const res = p.consume(msg, ec);
        if (ec)
            return true;

        if (!res)
            return false;

        adapter(res.value(), ec);
        if (ec)
            return true;
    }

    return true;
}

}  // namespace redis::resp3

#endif  // REDIS_RESP3_PARSER_HPP
