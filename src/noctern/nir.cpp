#include "./nir.hpp"

#include <bit>
#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstring>
#include <limits>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "noctern/enum.hpp"
#include "noctern/inout.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    namespace {
        double parse_double(std::string_view value) {
            double answer;
            auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), answer);
            assert(ptr == value.data() + value.size());
            assert(ec == std::errc {});
            return answer;
        }
    }

    struct nir::instructions::function_compiler {
        in<const tokens> source;
        inout<string_intern_table> global_names;

        out<std::vector<instruction>> instructions;
        out<std::vector<payload_word_t>> payloads;

        std::vector<std::string_view> register_names = {};
        std::unordered_map<std::string_view, register_index_t> registers = {};

        std::vector<register_index_t> expr_stack = {};

        template <opcode op, typename... Args>
        void write(val_t<op>, Args... args) {
            instructions->push_back(instruction {
                .opcode = op,
                .payload = encode(val<op>, args...),
            });
        }

        payload_index_t encode(val_t<opcode::return_>, register_index_t reg) {
            return reg;
        }

        payload_index_t encode(
            val_t<opcode::load_nonlocal>, register_index_t reg, interned_string global_name) {
            auto array = global_name.encode_as(type<payload_word_t>);
            auto index = static_cast<payload_index_t>(payloads->size());
            payloads->push_back(reg);
            payloads->insert(payloads->end(), array.begin(), array.end());
            return index;
        }

        payload_index_t encode(
            val_t<opcode::load_int_lit>, register_index_t reg, double int_literal) {
            assert(int_literal <= std::numeric_limits<payload_word_t>::max()
                && "internal error: Large literals not yet supported");

            auto index = static_cast<payload_index_t>(payloads->size());
            payloads->push_back(reg);
            payloads->push_back(static_cast<payload_word_t>(int_literal));
            return index;
        }

        payload_index_t encode(
            val_t<opcode::load_real_lit>, register_index_t reg, double real_literal) {
            auto index = static_cast<payload_index_t>(payloads->size());
            payloads->push_back(reg);

            constexpr auto Div = sizeof(double) / sizeof(payload_word_t);
            constexpr auto Rem = sizeof(double) % sizeof(payload_word_t);
            constexpr auto DivRoundingUp = Div + (Rem != 0 ? 1 : 0);
            constexpr auto Size = DivRoundingUp;

            // TODO: doubles and endianness? May not matter at all.
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(double)>>(real_literal);
            std::array<payload_word_t, Size> result = {};
            std::memcpy(reinterpret_cast<std::byte*>(result.data()), bytes.data(), bytes.size());
            payloads->insert(payloads->end(), result.begin(), result.end());

            return index;
        }

        payload_index_t encode(val_t<opcode::call>, register_index_t reg, register_index_t function,
            std::span<const register_index_t> args) {
            auto index = static_cast<payload_index_t>(payloads->size());
            payloads->push_back(reg);
            payloads->push_back(function);
            payloads->push_back(static_cast<payload_word_t>(args.size()));
            payloads->insert(payloads->end(), args.begin(), args.end());
            return index;
        }

        void compile_function(const tokens& source, tokens::const_iterator& pos) {
            while (source.id(*pos) != token_id::rparen) {
                assert(source.id(*pos) == token_id::ident);
                register_names.push_back(source.string(*pos));
                auto [it, was_inserted]
                    = registers.try_emplace(register_names.back(), register_names.size() - 1);

                // Error: duplicate function parameter names!
                assert(was_inserted && "Duplicated function parameter names");
                ++pos;
            }
            ++pos;

            token_id id = source.id(*pos);
            if (id == token_id::lbrace) {
                compile_block(source, pos);
            } else {
                register_index_t reg = compile_expr(source, pos);
                write(val<opcode::return_>, reg);
            }
        }

        void compile_block(const tokens& source, tokens::const_iterator& pos) {
            assert(source.id(*pos) == token_id::lbrace);
            ++pos;

            while (source.id(*pos) == token_id::let) {
                ++pos;
                assert(source.id(*pos) == token_id::ident);
                std::string_view variable_name = source.string(*pos);
                ++pos;

                register_index_t reg = compile_expr(source, pos);
                assert(register_names[reg] == "" && "internal error: unnamed register has a name?");

                register_names[reg] = variable_name;
                auto [it, was_inserted]
                    = registers.try_emplace(register_names.back(), register_names.size() - 1);
                // Error: duplicate variable names!
                assert(was_inserted && "Duplicated variable names");
            }

            assert(source.id(*pos) == token_id::return_);
            ++pos;
            register_index_t reg = compile_expr(source, pos);
            write(val<opcode::return_>, reg);
            assert(source.id(*pos) == token_id::rbrace);
            ++pos;
        }

        register_index_t compile_expr(const tokens& source, tokens::const_iterator& pos) {
            assert(expr_stack.empty());

            auto new_register = [&] -> register_index_t {
                auto reg = static_cast<register_index_t>(register_names.size());
                register_names.push_back("");
                return reg;
            };

            while (source.id(*pos) != token_id::semicolon) {
                token next = *pos;
                token_id id = source.id(next);
                ++pos;

                if (id == token_id::ident) {
                    std::string_view id = source.string(next);

                    auto it = registers.find(id);
                    if (it != registers.end()) {
                        // Local variable. No instructions necessary.
                        expr_stack.push_back(it->second);
                    } else {
                        interned_string global_id = global_names->intern(id);
                        write(val<opcode::load_nonlocal>, new_register(), global_id);
                    }
                } else if (id == token_id::int_lit) {
                    double d = parse_double(source.string(next));
                    register_index_t reg = new_register();
                    expr_stack.push_back(reg);
                    write(val<opcode::load_int_lit>, reg, d);
                } else if (id == token_id::real_lit) {
                    double d = parse_double(source.string(next));
                    register_index_t reg = new_register();
                    expr_stack.push_back(reg);
                    write(val<opcode::load_real_lit>, reg, d);
                } else if (id == token_id::plus || id == token_id::minus || id == token_id::mult
                    || id == token_id::div) {
                    assert(expr_stack.size() >= 2);
                    register_index_t second = expr_stack.back();
                    expr_stack.pop_back();
                    register_index_t first = expr_stack.back();
                    expr_stack.pop_back();

                    std::string_view operator_function_name
                        = enum_switch(id, []<token_id id>(val_t<id>) -> std::string_view {
                              if constexpr (id == token_id::plus) {
                                  return "add";
                              } else if constexpr (id == token_id::minus) {
                                  return "sub";
                              } else if constexpr (id == token_id::mult) {
                                  return "mul";
                              } else if constexpr (id == token_id::div) {
                                  return "div";
                              } else {
                                  assert(false && "not an operation");
                              }
                          });

                    auto operator_id = new_register();
                    write(val<opcode::load_nonlocal>, operator_id,
                        global_names->intern(operator_function_name));
                    register_index_t reg = new_register();
                    expr_stack.push_back(reg);
                    write(val<opcode::call>, reg, operator_id,
                        std::array<register_index_t, 2>({first, second}));
                }
            }
            ++pos;

            assert(expr_stack.size() == 1);
            double result = expr_stack.back();
            expr_stack.pop_back();
            return result;
        }
    };

    auto nir::instructions::compile_function(
        const tokens& source, token from, string_intern_table& global_names) -> function {
        const auto index = static_cast<function_index_t>(instructions_.size());

        function_compiler compiler {
            .source = in(source),
            .global_names = inout(global_names),
            .instructions = out(instructions_),
            .payloads = out(payloads_),
        };
        tokens::const_iterator pos = source.to_iterator(from);
        compiler.compile_function(source, pos);

        return function(index);
    }
}
