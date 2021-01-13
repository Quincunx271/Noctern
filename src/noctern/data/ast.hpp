#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <noctern/utils/debug_print.hpp>

namespace noctern::ast {
    struct identifier {
        std::string value;
    };

    struct basic_type {
        identifier value;
    };

    struct type;

    struct fn_type {
        std::unique_ptr<type> from;
        std::unique_ptr<type> to;
    };

    struct eval_type {
        basic_type base;
        std::vector<type> args;
    };

    using type_type = std::variant<basic_type, fn_type, eval_type>;

    struct type {
        type_type value;
    };

    struct int_lit_expr {
        // TODO: make unbounded
        std::int64_t value;
    };

    struct real_lit_expr {
        // TODO: make unbounded
        long double value;
    };

    struct string_lit_expr {
        std::string value;
    };

    struct expr;

    struct fn_call_expr {
        std::unique_ptr<expr> fn;
        std::vector<expr> args;
    };

    struct expr;

    struct lambda_expr {
        std::vector<identifier> params;
        std::unique_ptr<expr> body;
    };

    using expr_type = std::variant<int_lit_expr, real_lit_expr, string_lit_expr, fn_call_expr,
        lambda_expr, identifier>;

    struct expr {
        expr_type value;
    };

    struct fn_decl {
        identifier name;
        ast::type type;
        std::vector<identifier> params;
        expr body;
    };

    struct attribute_decl {
        identifier name;
        ast::type type;
    };

    struct struct_decl {
        identifier name;
        std::vector<attribute_decl> attributes;
    };

    using declaration_type = std::variant<struct_decl, fn_decl>;

    struct declaration {
        declaration_type value;
    };

    struct file {
        std::vector<declaration> value;
    };

    std::ostream& operator<<(std::ostream& lhs, debug_print<identifier> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<basic_type> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_type> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<eval_type> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<type> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<int_lit_expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<real_lit_expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<string_lit_expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_call_expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<lambda_expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<expr> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<fn_decl> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<attribute_decl> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<struct_decl> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<declaration> const& rhs);
    std::ostream& operator<<(std::ostream& lhs, debug_print<file> const& rhs);
}
