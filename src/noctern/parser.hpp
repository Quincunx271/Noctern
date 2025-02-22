#pragma once

#include <vector>

#include "noctern/tokenize.hpp"

namespace noctern {
    class parse_tree {
    public:
        class builder;

        explicit parse_tree(builder builder);

    private:
        friend class builder;

        std::vector<token> postorder_;
    };

    parse_tree parse(const tokens& input);
}