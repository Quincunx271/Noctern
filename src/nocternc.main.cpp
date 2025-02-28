#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#include <fmt/core.h>

#include "noctern/compilation_unit.hpp"
#include "noctern/interpreter.hpp"
#include "noctern/parser.hpp"
#include "noctern/symbol_table.hpp"
#include "noctern/tokenize.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        fmt::println(stderr, "Usage: nocternc <file.nct>");
        return 1;
    }

    // TODO: mmap
    std::FILE* file = std::fopen(argv[1], "rb");
    if (file == nullptr) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "Couldn't find file {}: {}", argv[1], err);
        return 1;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "fseek failed: {}", err);
        return 1;
    }
    long length = std::ftell(file);
    if (length == -1) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "ftell failed: {}", err);
        return 1;
    }
    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "fseek failed: {}", err);
        return 1;
    }
    std::string source(length, '\0');
    [[maybe_unused]] size_t c = std::fread(source.data(), source.size(), length, file);
    if (std::ferror(file) != 0) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "fread failed: {}", err);
        return 1;
    }
    if (std::feof(file) == 0) {
        std::string err(std::strerror(errno));
        fmt::println(stderr, "failed to read entire file; didn't find eof.");
        return 1;
    }

    noctern::tokens tokens = noctern::tokenize_all(source);
    tokens = noctern::parse(std::move(tokens));
    noctern::compilation_unit compile_unit(tokens);
    noctern::symbol_table symbol_table(tokens, compile_unit);

    std::optional<noctern::token> main = symbol_table.find_fn_decl("Main");
    if (!main.has_value()) {
        fmt::println(stderr, "No `Main()` function found!");
        return 1;
    }

    noctern::interpreter interpreter(std::move(symbol_table));
    double result = interpreter.eval_fn(tokens, *main, noctern::interpreter::frame {});

    fmt::println(stdout, "Result: {}", result);

    return 0;
}
