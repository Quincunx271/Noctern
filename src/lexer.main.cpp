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

    for (noctern::token token : tokens) {
        noctern::token_id id = tokens.id(token);
        if (id == noctern::token_id::space) continue;
        if (noctern::has_data(id)) {
            fmt::println("<{}: {}>", stringify(id), tokens.string(token));
        } else {
            fmt::println("<{}>", stringify(id));
        }
    }
}
