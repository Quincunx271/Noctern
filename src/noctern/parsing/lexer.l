%{
#include <cassert>
#include <cstddef>
#include <limits>
#include <noctern/parsing/lexer.hpp>

using noctern::token_type;

struct noctern_lexer_extra
{
    std::size_t current_position = 0;
    std::size_t token_length = 0;

    std::size_t full_current_position = 0;
    std::size_t full_token_length = 0;

    std::size_t bracket_count = 0;
    std::size_t variable_reference_depth = 0;
    std::size_t make_reference_depth = 0;

    token_type type = token_type::unknown;

    int start_condition = 0;

    void update_position(std::size_t length, std::size_t submatch_offset,
                         std::size_t submatch_length) {
        increment_position(length);
        current_position += submatch_offset;
        token_length = submatch_length;
    }

    void extend_match(std::size_t length) {
        token_length += length;
        full_token_length += length;
    }

    void extend_full_match(std::size_t length) {
        full_token_length += length;
    }

    void increment_position(std::size_t length) {
        full_current_position += full_token_length;
        full_token_length = length;

        current_position = full_current_position;
        token_length = length;
    }

    void increment_only_full_position(std::size_t length) {
        increment_position(length);
        token_length = 0;
    }

    int match_literal(token_type type, std::size_t length) {
        this->type = type;
        increment_position(length);
        token_length = 0;
        return 1;
    }

    int match_content(token_type type, std::size_t length, std::size_t content_start, std::size_t content_end) {
        this->type = type;
        update_position(length, content_start, content_end - content_start);
        return 1;
    }
};
%}

%option extra-type="noctern_lexer_extra"
%option prefix="noctern_lexer_impl"
%option reentrant
%option noyywrap
%option nounistd
%option never-interactive
%option pointer

%x STRING
%%
\n              return yyextra.match_literal(token_type::newline, yyleng);

\(              return yyextra.match_literal(token_type::lcall, yyleng);

\)              return yyextra.match_literal(token_type::rcall, yyleng);

\[              return yyextra.match_literal(token_type::ltcall, yyleng);

\]              return yyextra.match_literal(token_type::rtcall, yyleng);

\{              return yyextra.match_literal(token_type::lblock, yyleng);

\}              return yyextra.match_literal(token_type::rblock, yyleng);

\.\(            return yyextra.match_literal(token_type::lindex, yyleng);

::              return yyextra.match_literal(token_type::typed_as, yyleng);

\.              return yyextra.match_literal(token_type::member, yyleng);

\.\.            return yyextra.match_literal(token_type::compose, yyleng);

\$              return yyextra.match_literal(token_type::curry, yyleng);

->              return yyextra.match_literal(token_type::farrow, yyleng);

\\              return yyextra.match_literal(token_type::lambda, yyleng);

\+              return yyextra.match_literal(token_type::plus, yyleng);

-               return yyextra.match_literal(token_type::minus, yyleng);

\*              return yyextra.match_literal(token_type::times, yyleng);

\/              return yyextra.match_literal(token_type::divide, yyleng);

mod             return yyextra.match_literal(token_type::modulo, yyleng);

rem             return yyextra.match_literal(token_type::remainder, yyleng);

==              return yyextra.match_literal(token_type::eq, yyleng);

!=              return yyextra.match_literal(token_type::ne, yyleng);

\<              return yyextra.match_literal(token_type::lt, yyleng);

\<=             return yyextra.match_literal(token_type::le, yyleng);

>               return yyextra.match_literal(token_type::gt, yyleng);

>=              return yyextra.match_literal(token_type::ge, yyleng);

\<=>            return yyextra.match_literal(token_type::cmp, yyleng);

and             return yyextra.match_literal(token_type::l_and, yyleng);

or              return yyextra.match_literal(token_type::l_or, yyleng);

not             return yyextra.match_literal(token_type::l_not, yyleng);

b&              return yyextra.match_literal(token_type::b_and, yyleng);

b\|             return yyextra.match_literal(token_type::b_or, yyleng);

b~              return yyextra.match_literal(token_type::b_not, yyleng);

b\^             return yyextra.match_literal(token_type::b_xor, yyleng);

def             return yyextra.match_literal(token_type::def_fn, yyleng);

val             return yyextra.match_literal(token_type::def_val, yyleng);

struct          return yyextra.match_literal(token_type::def_struct, yyleng);

typealias       return yyextra.match_literal(token_type::def_typealias, yyleng);

import          return yyextra.match_literal(token_type::import_, yyleng);

return          return yyextra.match_literal(token_type::return_, yyleng);

[a-zA-Z_][A-Za-z0-9_]*      return yyextra.match_content(token_type::identifier, yyleng, 0, yyleng);

[0-9]+\.[0-9]*|\.[0-9]+     return yyextra.match_content(token_type::real, yyleng, 0, yyleng);

[0-9]+                      return yyextra.match_content(token_type::integer, yyleng, 0, yyleng);

    /* All the string-parsing stuff */
\" {
    yyextra.increment_position(yyleng);
    yyextra.current_position += yyleng;
    yyextra.type = token_type::string;
    BEGIN(STRING);
}

<STRING>([^\\\"]|\\.)+ {
    yyextra.extend_match(yyleng);
}

<STRING>\" {
    yyextra.extend_full_match(yyleng);
    yyextra.token_length -= yyleng;
    BEGIN(INITIAL);
    return 1;
}

<STRING><<EOF>> {
    yyextra.type = token_type::unterminated_string;
    BEGIN(INITIAL);
    return 1;
}

<STRING>. {
    yyextra.extend_full_match(yyleng);
    yyextra.type = token_type::unknown;
    BEGIN(INITIAL);
    return 1;
}
    /* string stuff end */

[ \t\r]+    return yyextra.match_content(token_type::space, yyleng, 0, yyleng);

\/\/[^\n]*  return yyextra.match_content(token_type::line_comment, yyleng, 2, yyleng - 1);

<INITIAL>.  return yyextra.match_content(token_type::unknown, yyleng, 0, yyleng);

<<EOF>> {
    yyextra.increment_position(0);
    yyextra.type = token_type::end_of_file;
    return 0;
}
%%

namespace {
    void join_token(noctern::token& lhs, noctern::token const& rhs) {
        lhs.text = std::string_view {
            lhs.text.data(),
            lhs.text.size() + rhs.text.size(),
        };
        lhs.full_text = std::string_view {
            lhs.full_text.data(),
            lhs.full_text.size() + rhs.full_text.size(),
        };
    }
    noctern::token extract_token(
        std::string_view input,
        noctern_lexer_extra const& extra)
    {
        return noctern::token {
            input.substr(extra.current_position, extra.token_length),
            extra.type,
            input.substr(extra.full_current_position, extra.full_token_length),
        };
    }
    std::optional<noctern::token>
    read_token(std::string_view input, void* lexer)
    {
        bool success = yylex(lexer) != 0;
        if (!success) return std::nullopt;
        auto const extra = yyget_extra(lexer);
        return ::extract_token(input, extra);
    }
}

namespace noctern {
    lexer::lexer(std::string_view input)
        : input_{input}
    {
        yylex_init(&lexer_);
        assert(input_.size() < std::numeric_limits<int>::max());
        yy_scan_bytes(input_.data(), static_cast<int>(input_.size()), lexer_);
        advance();
    }
    lexer::~lexer()
    {
        yylex_destroy(lexer_);
        lexer_ = nullptr;
    }
    void lexer::advance()
    {
        current_token_ = ::read_token(input_, lexer_);
    }
    bool lexer::has_next() const
    {
        return current_token_.has_value();
    }
    token const& lexer::read() const
    {
        return *current_token_;
    }
}
