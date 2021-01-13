%{
#include <cassert>
#include <cstddef>
#include <limits>
#include <noctern/parsing/lexer.hpp>

struct noctern_lexer_extra
{
    std::size_t current_position = 0;
    std::size_t token_length = 0;

    std::size_t full_current_position = 0;
    std::size_t full_token_length = 0;

    std::size_t bracket_count = 0;
    std::size_t variable_reference_depth = 0;
    std::size_t make_reference_depth = 0;

    noctern::token_type type = noctern::token_type::unknown;

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

    void match_literal(std::size_t length) {
        increment_position(length);
        token_length = 0;
    }

    void match_content(std::size_t length, std::size_t content_start, std::size_t content_end) {
        update_position(length, content_start, content_end - content_start);
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
\n {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::newline;
    return 1;
}

\( {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::lcall;
    return 1;
}

\) {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::rcall;
    return 1;
}

\[ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::ltcall;
    return 1;
}

\] {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::rtcall;
    return 1;
}

\{ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::lblock;
    return 1;
}

\} {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::rblock;
    return 1;
}

\.\( {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::lindex;
    return 1;
}

:: {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::typed_as;
    return 1;
}

\. {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::member;
    return 1;
}

\.\. {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::compose;
    return 1;
}

\$ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::curry;
    return 1;
}

-> {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::farrow;
    return 1;
}

\\ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::lambda;
    return 1;
}

\+ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::plus;
    return 1;
}

- {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::minus;
    return 1;
}

\* {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::times;
    return 1;
}

\/ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::divide;
    return 1;
}

mod {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::modulo;
    return 1;
}

rem {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::remainder;
    return 1;
}

== {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::eq;
    return 1;
}

!= {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::ne;
    return 1;
}

\< {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::lt;
    return 1;
}

\<= {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::le;
    return 1;
}

> {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::gt;
    return 1;
}

>= {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::ge;
    return 1;
}

\<=> {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::cmp;
    return 1;
}

and {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::l_and;
    return 1;
}

or {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::l_or;
    return 1;
}

not {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::l_not;
    return 1;
}

b& {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::b_and;
    return 1;
}

b\| {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::b_or;
    return 1;
}

b~ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::b_not;
    return 1;
}

b\^ {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::b_xor;
    return 1;
}

def {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::def_fn;
    return 1;
}

val {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::def_val;
    return 1;
}

struct {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::def_struct;
    return 1;
}

typealias {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::def_typealias;
    return 1;
}

import {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::import_;
    return 1;
}

return {
    yyextra.match_literal(yyleng);
    yyextra.type = noctern::token_type::return_;
    return 1;
}

[a-zA-Z_][A-Za-z0-9_]* {
    yyextra.match_content(yyleng, 0, yyleng);
    yyextra.type = noctern::token_type::identifier;
    return 1;
}

    /* String, Integer, Real */
[0-9]+\.[0-9]*|\.[0-9]+ {
    yyextra.match_content(yyleng, 0, yyleng);
    yyextra.type = noctern::token_type::real;
    return 1;
}

[0-9]+ {
    yyextra.match_content(yyleng, 0, yyleng);
    yyextra.type = noctern::token_type::integer;
    return 1;
}

    /* All the string-parsing stuff */
\" {
    yyextra.increment_position(yyleng);
    yyextra.current_position += yyleng;
    yyextra.type = noctern::token_type::string;
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
    yyextra.type = noctern::token_type::unterminated_string;
    BEGIN(INITIAL);
    return 1;
}

<STRING>. {
    yyextra.extend_full_match(yyleng);
    yyextra.type = noctern::token_type::unknown;
    BEGIN(INITIAL);
    return 1;
}
    /* string stuff end */

[ \t\r]+ {
    yyextra.increment_position(yyleng);
    yyextra.type = noctern::token_type::space;
    return 1;
}

\/\/[^\n]* {
    yyextra.match_content(yyleng, 2, yyleng - 1);
    yyextra.type = noctern::token_type::line_comment;
    return 1;
}

<INITIAL>. {
    yyextra.increment_position(yyleng);
    yyextra.type = noctern::token_type::unknown;
    return 1;
}

<<EOF>> {
    yyextra.increment_position(0);
    yyextra.type = noctern::token_type::end_of_file;
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
