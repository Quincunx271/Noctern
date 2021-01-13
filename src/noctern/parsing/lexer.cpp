#include "./lexer.hpp"

#include <ostream>

namespace noctern {
    std::ostream& operator<<(std::ostream& out, debug_print<lexer::sentinel> const&)
    {
        return out << "<<eof>>";
    }

    std::ostream& operator<<(std::ostream& out, debug_print<lexer::iterator> const& iter)
    {
        if (iter.value == lexer::sentinel{}) {
            return out << lexer::sentinel{};
        }
        return out << "<iterator." << iter.value.lexer_ << '>';
    }
}
