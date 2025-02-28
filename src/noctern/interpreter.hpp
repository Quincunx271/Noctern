#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "noctern/symbol_table.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    class interpreter {
    public:
        struct frame {
            std::unordered_map<std::string_view, double> locals;
            std::vector<double> expr_stack;
        };

        explicit interpreter(symbol_table table)
            : table_(std::move(table)) {
        }

        double eval_fn(const tokens& source, token from, frame arguments) const;

    private:
        double eval_block(const tokens& source, frame& frame, tokens::const_iterator& pos) const;

        double eval_expr(const tokens& source, frame& frame, tokens::const_iterator& pos) const;

        symbol_table table_;
    };
}
