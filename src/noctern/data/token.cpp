#include "./token.hpp"

#include <cassert>
#include <ostream>
#include <string_view>

#include <frozen/map.h>

namespace noctern {
    namespace {
        constexpr auto type_stringify = frozen::make_map<token_type, std::string_view>({
            {token_type::unknown, "unknown"},
            {token_type::space, "space"},
            {token_type::newline, "newline"},
            {token_type::line_comment, "line_comment"},
            {token_type::lcall, "call_open"},
            {token_type::rcall, "call_close"},
            {token_type::ltcall, "tcall_open"},
            {token_type::rtcall, "tcall_close"},
            {token_type::lblock, "block_open"},
            {token_type::rblock, "block_close"},
            {token_type::lindex, "index_open"},
            // {token_type::rindex, "index_close"},
            // {token_type::lgroup, "lparen"},
            // {token_type::rgroup, "rparen"},
            {token_type::typed_as, "typed_as"},
            // {token_type::lattr_list, "attr_list_open"},
            // {token_type::rattr_list, "attr_list_close"},
            {token_type::bind, "bind"},
            {token_type::list_sep, "list_sep"},
            {token_type::stmt_term, "stmt_term"},

            {token_type::member, "member"},
            {token_type::compose, "compose"},
            {token_type::curry, "curry"},
            {token_type::farrow, "farrow"},
            {token_type::lambda, "lambda"},

            {token_type::plus, "add"},
            {token_type::minus, "sub"},
            {token_type::times, "mul"},
            {token_type::divide, "div"},
            {token_type::modulo, "mod"},
            {token_type::remainder, "rem"},
            {token_type::eq, "eq"},
            {token_type::ne, "ne"},
            {token_type::lt, "lt"},
            {token_type::le, "le"},
            {token_type::gt, "gt"},
            {token_type::ge, "ge"},
            {token_type::cmp, "cmp"},
            {token_type::l_and, "logical_and"},
            {token_type::l_or, "logical_or"},
            {token_type::l_not, "logical_not"},

            {token_type::b_and, "bitwise_and"},
            {token_type::b_or, "bitwise_or"},
            {token_type::b_not, "bitwise_not"},
            {token_type::b_xor, "bitwise_xor"},

            {token_type::def_fn, "def_fn"},
            {token_type::def_val, "def_val"},
            {token_type::def_struct, "def_struct"},
            {token_type::def_typealias, "def_typealias"},

            {token_type::import_, "import"},
            {token_type::return_, "return"},

            {token_type::identifier, "identifier"},
            {token_type::string, "string"},
            {token_type::integer, "integer"},
            {token_type::real, "real"},

            {token_type::unterminated_string, "unterminated_string"},
            {token_type::end_of_file, "end_of_file"},
        });
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<token_type> const& rhs) {
        auto const lookup = type_stringify.find(rhs.value);
        assert(lookup != type_stringify.end());

        return lhs << lookup->second;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<token> const& rhs) {
        return lhs << '<' << noctern::debug_print(rhs.value.type) << ": \"" << rhs.value.text
                   << "\">";
    }
}
