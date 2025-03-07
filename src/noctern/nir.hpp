#pragma once

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

#include "noctern/enum.hpp"
#include "noctern/intern_table.hpp"
#include "noctern/tokenize.hpp"

namespace noctern {
    // NIR - Noctern IR
    //
    // This is a stripped down intermediate representation of the language frontend. The idea is
    // that all frontend work should be handled by this IR.
    //
    // Some principles:
    //  - Ease of implementation/use over expressiveness.
    //    - No nested expressions. No unnamed temporaries.
    //  - Explicit over implicit.
    //    - No implicit conversions.
    //    - Operators are just functions with special names.
    class nir {
    public:
        class instructions {
            using function_index_t = token_index_t;

        public:
            class function {
                friend instructions;

            public:
                constexpr function() = default;

            private:
                explicit constexpr function(function_index_t index)
                    : index_(index) {
                }

                function_index_t index_ = static_cast<function_index_t>(-1);
            };

            enum class opcode : uint8_t {
#define NOCTERN_X_NIR_OPCODE(X)                                                                    \
    X(nop)                                                                                         \
    X(load_nonlocal)                                                                               \
    X(load_int_lit)                                                                                \
    X(load_real_lit)                                                                               \
    X(call)                                                                                        \
    X(return_)
#define NOCTERN_MAKE_ENUM(name) name,
                NOCTERN_X_NIR_OPCODE(NOCTERN_MAKE_ENUM)
#undef NOCTERN_MAKE_ENUM
            };

            function compile_function(
                const tokens& source, token from, string_intern_table& global_names);

            template <typename Fn>
            void visit(function func, Fn&& fn) const {
                const std::span<const payload_word_t> payload_span(payloads_);
                for (instruction instruction : std::span(instructions_).subspan(func.index_)) {
                    enum_switch(instruction.opcode, [&]<opcode opcode>(val_t<opcode> op) {
                        using enum instructions::opcode;
                        if constexpr (op == nop) {
                            std::invoke(fn, op, -1);
                        } else if constexpr (op == return_) {
                            std::invoke(fn, op, register_index_t {instruction.payload});
                        } else if constexpr (op == load_nonlocal) {
                            auto data = payload_span.subspan(
                                instruction.payload, payloads_.size() - instruction.payload);

                            register_index_t reg = data[0];
                            data = data.subspan(1, data.size() - 1);

                            auto name = interned_string::decode_from(data);

                            std::invoke(fn, op, reg, name);
                        } else if constexpr (op == load_int_lit) {
                            std::invoke(fn, op, register_index_t {payloads_[instruction.payload]},
                                payload_word_t {payloads_[instruction.payload + 1]});
                        } else if constexpr (op == load_real_lit) {
                            auto data = payload_span.subspan(
                                instruction.payload, payloads_.size() - instruction.payload);

                            register_index_t reg = data[0];
                            data = data.subspan(1, data.size() - 1);

                            // TODO: doubles and endianness? May not matter at all.
                            std::array<std::byte, sizeof(double)> bytes = {};
                            std::memcpy(bytes.data(),
                                reinterpret_cast<const std::byte*>(data.data()), bytes.size());

                            std::invoke(fn, op, reg, std::bit_cast<double>(bytes));
                        } else {
                            static_assert(op == call);

                            auto data = payload_span.subspan(
                                instruction.payload, payloads_.size() - instruction.payload);

                            register_index_t reg = data[0];
                            register_index_t function = data[1];
                            payload_word_t num_args = data[2];
                            auto args = data.subspan(3, num_args);

                            std::invoke(fn, op, reg, function, args);
                        }
                    });
                }
            }

        private:
            friend enum_mixin;

            template <typename Fn>
            friend constexpr decltype(auto) switch_introspect(opcode op, Fn&& fn) {
                switch (op) {
                    using enum opcode;
                    NOCTERN_X_NIR_OPCODE(NOCTERN_ENUM_X_INTROSPECT)
                }
                assert(false);
            }

            template <typename Fn>
            friend constexpr decltype(auto) introspect(type_t<opcode>, Fn&& fn) {
                using enum opcode;
                return std::invoke(std::forward<Fn>(fn)
#define NOCTERN_NIR_OPCODE_TYPE(name) , val<name>
                        NOCTERN_X_NIR_OPCODE(NOCTERN_NIR_OPCODE_TYPE)
#undef NOCTERN_NIR_OPCODE_TYPE
                );
            }
#undef NOCTERN_X_NIR_OPCODE

        private:
            struct function_compiler;

            using payload_index_t = uint32_t;
            using payload_word_t = uint32_t;
            using register_index_t = payload_word_t;

            struct instruction {
                instructions::opcode opcode : 8;
                payload_index_t payload : 24;
            };

            std::vector<instruction> instructions_;
            std::vector<payload_word_t> payloads_;

            std::vector<std::vector<std::string_view>> variable_names_;
        };

    private:
        instructions instructions_;
        std::vector<instructions::function> functions_;
    };
}