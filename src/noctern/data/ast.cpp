#include "./ast.hpp"

#include <ostream>

#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fmt/ranges.h>

namespace noctern::ast {
    std::ostream& operator<<(std::ostream& lhs, debug_print<identifier> const& rhs) {
        fmt::print(lhs, "id({})", rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<basic_type> const& rhs) {
        fmt::print(lhs, "basic_type({})", rhs.value.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_type> const& rhs) {
        fmt::print(lhs, "fn_type({} -> {})", *rhs.value.from, *rhs.value.to);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<eval_type> const& rhs) {
        fmt::print(lhs, "eval_type({}[{}])", debug_print(rhs.value.base),
            debug_fmt_join(rhs.value.args, ", "));
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<type> const& rhs) {
        std::visit(
            [&](auto const& value) { fmt::print(lhs, "{}", debug_print(value)); }, rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<int_lit_expr> const& rhs) {
        fmt::print(lhs, "int({})", rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<real_lit_expr> const& rhs) {
        fmt::print(lhs, "real({})", rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<string_lit_expr> const& rhs) {
        fmt::print(lhs, "string({})", rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_call_expr> const& rhs) {
        fmt::print(lhs, "fn_call({}({}))", debug_print(*rhs.value.fn),
            debug_fmt_join(rhs.value.args, ", "));
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<lambda_expr> const& rhs) {
        return lhs << debug_print(*rhs.value.value);
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<expr> const& rhs) {
        std::visit(
            [&](auto const& value) { fmt::print(lhs, "{}", debug_print(value)); }, rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_decl> const& rhs) {
        fmt::print(lhs, "fn_decl({}, {}) {} = {}", debug_print(rhs.value.type),
            debug_print(rhs.value.name), debug_fmt_join(rhs.value.params, ", "),
            debug_print(rhs.value.body));
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<attribute_decl> const& rhs) {
        fmt::print(lhs, "{} :: {}", debug_print(rhs.value.name), debug_print(rhs.value.type));
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<struct_decl> const& rhs) {
        fmt::print(lhs, "struct({}):\n{}", debug_print(rhs.value.name),
            debug_fmt_join(rhs.value.attributes, ",\n"));
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<declaration> const& rhs) {
        std::visit(
            [&](auto const& value) { fmt::print(lhs, "{}", debug_print(value)); }, rhs.value.value);
        return lhs;
    }

    std::ostream& operator<<(std::ostream& lhs, debug_print<file> const& rhs) {
        for (auto const& x : rhs.value.value) {
            lhs << debug_print(x) << '\n';
        }
        return lhs;
    }
}
