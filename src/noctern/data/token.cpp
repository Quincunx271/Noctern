#include "./token.hpp"

#include <cassert>
#include <ostream>
#include <string_view>

namespace noctern {
    namespace {
        constexpr std::string_view type_stringify(token_type type) {
            switch (type) {
            case token_type::unknown: return "unknown";
            case token_type::space: return "space";
            case token_type::newline: return "newline";
            case token_type::line_comment: return "line_comment";
            case token_type::lcall: return "call_open";
            case token_type::rcall: return "call_close";
            case token_type::ltcall: return "tcall_open";
            case token_type::rtcall: return "tcall_close";
            case token_type::lblock: return "block_open";
            case token_type::rblock: return "block_close";
            case token_type::lindex: return "index_open";
            // case token_type::rindex: return "index_close";
            // case token_type::lgroup: return "lparen";
            // case token_type::rgroup: return "rparen";
            case token_type::typed_as: return "typed_as";
            // case token_type::lattr_list: return "attr_list_open";
            // case token_type::rattr_list: return "attr_list_close";
            case token_type::bind: return "bind";
            case token_type::list_sep: return "list_sep";
            case token_type::stmt_term: return "stmt_term";

            case token_type::member: return "member";
            case token_type::compose: return "compose";
            case token_type::curry: return "curry";
            case token_type::farrow: return "farrow";
            case token_type::lambda: return "lambda";

            case token_type::plus: return "add";
            case token_type::minus: return "sub";
            case token_type::times: return "mul";
            case token_type::divide: return "div";
            case token_type::modulo: return "mod";
            case token_type::remainder: return "rem";
            case token_type::eq: return "eq";
            case token_type::ne: return "ne";
            case token_type::lt: return "lt";
            case token_type::le: return "le";
            case token_type::gt: return "gt";
            case token_type::ge: return "ge";
            case token_type::cmp: return "cmp";
            case token_type::l_and: return "logical_and";
            case token_type::l_or: return "logical_or";
            case token_type::l_not: return "logical_not";

            case token_type::b_and: return "bitwise_and";
            case token_type::b_or: return "bitwise_or";
            case token_type::b_not: return "bitwise_not";
            case token_type::b_xor: return "bitwise_xor";

            case token_type::def_fn: return "def_fn";
            case token_type::def_val: return "def_val";
            case token_type::def_struct: return "def_struct";
            case token_type::def_typealias: return "def_typealias";

            case token_type::import_: return "import";
            case token_type::return_: return "return";

            case token_type::identifier: return "identifier";
            case token_type::string: return "string";
            case token_type::integer: return "integer";
            case token_type::real: return "real";

            case token_type::unterminated_string: return "unterminated_string";
            case token_type::end_of_file: return "end_of_file";
            }
            assert(false && "Unreachable!");
        }
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<token_type> const& rhs) {
        return lhs << type_stringify(rhs.value);
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<token> const& rhs) {
        return lhs << '<' << noctern::debug_print(rhs.value.type) << ": \"" << rhs.value.text
                   << "\">";
    }
}
