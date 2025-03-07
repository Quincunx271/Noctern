#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>

#include "noctern/compilation_unit.hpp"
#include "noctern/intern_table.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    class symbol_table {
    public:
        explicit symbol_table(const tokens& input, const compilation_unit& unit,
            string_intern_table& string_interner);

        std::optional<token> find_fn_decl(interned_string name) const {
            auto it = fn_table_.find(name);
            if (it == fn_table_.end()) return std::nullopt;
            return it->second;
        }

    private:
        // TODO: use a better map type.
        std::unordered_map<interned_string, token> fn_table_;
    };
}