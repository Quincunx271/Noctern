#pragma once

#include <fmt/core.h>
#include <iosfwd>

namespace noctern {
    template <typename T>
    struct debug_print {
        T const& value;

        debug_print(T const& value)
            : value {value} {
        }
    };

    template <typename Range, typename Sep>
    struct debug_fmt_join {
        Range const& range;
        Sep const& sep;

        debug_fmt_join(Range const& range, Sep const& sep)
            : range(range)
            , sep(sep) {
        }
    };
}

namespace fmt {
    template <typename Range, typename Sep>
    struct formatter<noctern::debug_fmt_join<Range, Sep>> {
        constexpr auto parse(format_parse_context& ctx) {
            return ctx.begin();
        }

        template <typename FormatContext>
        auto format(const noctern::debug_fmt_join<Range, Sep>& x, FormatContext& ctx) {
            auto pos = ctx.out();

            bool first = true;
            for (auto const& item : x.range) {
                if (!first) pos = fmt::format_to(pos, "{}", x.sep);
                pos = fmt::format_to(pos, "{}", noctern::debug_print(item));
                first = true;
            }

            return pos;
        }
    };
}
