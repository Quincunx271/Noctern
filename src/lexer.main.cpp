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

    tokens.walk([](noctern::token_id token_id, std::string_view str_data) {
        if (token_id == noctern::token_id::space) return;
        if (noctern::has_data(token_id)) {
            fmt::println("<{}: {}>", stringify(token_id), str_data);
        } else {
            fmt::println("<{}>", stringify(token_id));
        }
    });
}
