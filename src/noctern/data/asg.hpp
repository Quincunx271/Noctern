#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace noctern::asg {
    struct expression_id {
        std::size_t value;
    };

    struct global_id {
        std::size_t value;
    };

    enum class op_type {
        id,
        // brings in a global.
        // requires knowledge of what global was brought in; sources in the graph
        named_expr,
        // i.e. lambdas.
        // only requires basic graph info
        fn_expr,
        // only requires basic graph info
        fn_call,
        // declares a variable for this scope
        val_expr,
        // declares a type
        type_expr,
    };

    struct operation_id {
        op_type kind;
        std::size_t value;
    };

    struct expression {
        std::vector<expression_id> subexprs;
        operation_id operation;
        expression_id type;
    };

    struct global {
        std::string name;
        expression_id value;
    };

    struct module_ {
        std::vector<expression> expressions;
        // Includes all types and functions as well; types and functions are sugar.
        std::vector<global> globals;
    };
}
