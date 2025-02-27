#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>

#include "noctern/compilation_unit.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    class symbol_table {
    public:
        explicit symbol_table(const tokens& input, const compilation_unit& unit);

        std::optional<token> find_fn_decl(std::string_view name) const {
            auto it = fn_table_.find(name);
            if (it == fn_table_.end()) return std::nullopt;
            return it->second;
        }

    private:
        // TODO: use a better map type.
        std::unordered_map<std::string_view, token> fn_table_;
    };
}