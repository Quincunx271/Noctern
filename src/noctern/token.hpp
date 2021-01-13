#pragma once

#include <string_view>
#include <tuple>
#include <variant>

#include <noctern/debug_print.hpp>

namespace noctern {
    enum class token_type {
        unknown,
        space,
        newline,
        line_comment,

        lcall, // (
        rcall, // )
        ltcall, // [
        rtcall, // ]
        lblock, // {
        rblock, // }
        lindex, // .(
        rindex = rcall, // )
        typed_as, // :: (haskell's syntax for now)

        member, // .
        compose, // ..
        curry, // $
        farrow, // ->
        lambda, // backslash; looks like a lambda

        plus,
        minus,
        times,
        divide,
        modulo,
        remainder,
        eq,
        ne,
        lt,
        le,
        gt,
        ge,
        cmp, // <=>
        l_and,
        l_or,
        l_not,

        b_and,
        b_or,
        b_not,
        b_xor,

        def_fn,
        def_val,
        def_struct,
        def_typealias,

        import_,
        return_,

        identifier,
        string,
        integer, // literal
        real, // literal

        unterminated_string,
        end_of_file,
    };

    std::ostream& operator<<(std::ostream& lhs, debug_print<token_type> const& rhs);

    struct token {
        std::string_view text;
        token_type type;
        // Not salient. Mostly determinable from `text` and `type`.
        std::string_view full_text;
    };

    inline bool operator==(token const& lhs, token const& rhs) {
        // Compare types first; that's the cheaper comparison
        return std::tie(lhs.type, lhs.text) == std::tie(rhs.type, rhs.text);
    }

    inline bool operator!=(token const& lhs, token const& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<token> const& rhs);
}
