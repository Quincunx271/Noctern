#include <iostream>
#include <sstream>
#include <string>

#include <fmt/core.h>
#include <noctern/tokenize.hpp>

int main() {
    const std::string input = [] {
        std::ostringstream out;
        out << std::cin.rdbuf();
        return out.str();
    }();

    noctern::tokens tokens = noctern::tokenize_all(input);

    tokens.walk([](noctern::token token, std::string_view str_data) {
        if (token == noctern::token::space) return;
        if (noctern::has_data(token)) {
            fmt::println("<{}: {}>", stringify(token), str_data);
        } else {
            fmt::println("<{}>", stringify(token));
        }
    });
}
