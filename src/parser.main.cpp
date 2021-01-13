#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <noctern/parsing/lexer.hpp>
#include <noctern/parsing/parser.hpp>

int main() {
    auto const input = [] {
        std::ostringstream out;
        out << std::cin.rdbuf();
        return out.str();
    }();

    noctern::lexer lex {input};
    std::vector<noctern::token> tokens(lex.begin(), lex.end());
    std::erase_if(tokens, [](noctern::token const& tok) {
        return tok.type == noctern::token_type::space || tok.type == noctern::token_type::newline
            || tok.type == noctern::token_type::line_comment;
    });

    noctern::ast::file result = noctern::parse(tokens);

    std::cout << noctern::debug_print(result) << '\n';
}
