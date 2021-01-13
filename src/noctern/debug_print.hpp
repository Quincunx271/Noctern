#pragma once

#include <iosfwd>

namespace noctern {
    template <typename T>
    struct debug_print {
        T const& value;

        debug_print(T const& value)
            : value {value} {
        }
    };
}
