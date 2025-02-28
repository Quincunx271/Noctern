#include "./compilation_unit.hpp"

#include <ranges>
#include <span>

#include "noctern/tokenize.hpp"

namespace noctern {
    namespace {
        template <typename Range>
            requires std::ranges::input_range<Range>
        std::vector<token> from_range(Range&& range) {
            return std::vector<token>(std::ranges::begin(range), std::ranges::end(range));
        }
    }

    compilation_unit::compilation_unit(const tokens& input)
        : fn_defs_(noctern::from_range(std::ranges::filter_view(
              input, [&](token t) { return input.id(t) == token_id::fn_intro; }))) {
    }
}