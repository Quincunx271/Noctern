#include <iostream>
#include <sstream>
#include <string>

#include <noctern/parsing/lexer.hpp>

int main() {
    auto const input = [] {
        std::ostringstream out;
        out << std::cin.rdbuf();
        return out.str();
    }();

    noctern::lexer lex {input};

    for (auto const& token : lex) {
        std::cout << noctern::debug_print(token) << '\n';
    }
}
