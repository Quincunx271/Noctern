#pragma once

#include <span>
#include <vector>

#include "noctern/tokenize.hpp"

namespace noctern {
    class compilation_unit {
    public:
        explicit compilation_unit(const tokens& input, std::span<const token> postorder);

        std::span<const token> fn_defs() const {
            return fn_defs_;
        }

    private:
        std::vector<token> fn_defs_;
    };
}