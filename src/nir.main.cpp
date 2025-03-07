#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ranges>
#include <string>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include "noctern/compilation_unit.hpp"
#include "noctern/intern_table.hpp"
#include "noctern/interpreter.hpp"
#include "noctern/nir.hpp"
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
    noctern::string_intern_table global_symbols;
    noctern::symbol_table symbol_table(tokens, compile_unit, global_symbols);

    std::optional<noctern::token> main = symbol_table.find_fn_decl(global_symbols.intern("Main"));
    if (!main.has_value()) {
        fmt::println(stderr, "No `Main()` function found!");
        return 1;
    }

    noctern::nir::instructions instructions;
    noctern::nir::instructions::function fn
        = instructions.compile_function(tokens, *main, global_symbols);

    instructions.visit(fn,
        [&]<noctern::nir::instructions::opcode op, typename Register, typename... Args>(
            noctern::val_t<op>, Register reg, Args&&... args) {
            using enum noctern::nir::instructions::opcode;
            if constexpr (op == return_) {
                fmt::println("  return {}", reg);
            } else {
                fmt::print("  %r{} = {}", reg, stringify(op));

                if constexpr (op == load_int_lit || op == load_real_lit) {
                    fmt::print(" {}", [](auto x) { return x; }(args...));
                } else if constexpr (op == load_nonlocal) {
                    fmt::print(" {}", [&](noctern::interned_string id) {
                        return global_symbols.get(id);
                    }(args...));
                } else if constexpr (op == call) {
                    [&]<typename Reg>(Reg fn, std::span<const Reg> args) {
                        fmt::print(" %r{}({})", fn,
                            fmt::join(std::ranges::transform_view(
                                          args, [](auto reg) { return fmt::format("%r{}", reg); }),
                                ", "));
                    }(args...);
                }

                fmt::println("");
            }
        });

    return 0;
}
