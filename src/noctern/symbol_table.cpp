#include "./symbol_table.hpp"

#include <ranges>

#include "noctern/compilation_unit.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    namespace {
        namespace views = std::ranges::views;

        template <typename Range>
            requires std::ranges::input_range<Range>
        auto from_range(Range&& range) {
            return std::unordered_map(std::ranges::begin(range), std::ranges::end(range));
        }
    }

    symbol_table::symbol_table(const tokens& input, const compilation_unit& unit)
        : fn_table_(from_range(unit.fn_defs() | views::transform([&](token token) {
            const auto it = input.to_iterator(token);
            assert(input.id(it[1]) == token_id::ident);
            return std::pair(input.string(it[1]), it[2]);
        }))) {
    }
}