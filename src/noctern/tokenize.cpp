#include "./tokenize.hpp"

#include <algorithm>
#include <cassert>

namespace noctern {
    namespace {
        template <token_id token_id>
        using token_data_t = std::remove_cvref_t<decltype(token_data<token_id>)>;

        // This concept makes the static_assert print the name of the token_id with no defined data.
        template <token_id token_id>
        concept has_defined_token_data = !std::same_as<token_data_t<token_id>, std::nullptr_t>;

        // Ensure that token_data<> is specialized for all tokens.
        static_assert(enum_values(type<token_id>, []<token_id... tokens>(val_t<tokens>...) {
            static_assert((has_defined_token_data<tokens> && ...));
            return true;
        }));

        // Calls `fn(val<token_id>, token_data<token_id>)` once per empty token_id.
        template <typename Fn>
        constexpr void for_each_empty_token(Fn&& fn) {
            enum_values(type<token_id>, [&]<token_id... tokens>(val_t<tokens>...) {
                (
                    [&]<token_id token_id, typename Data>(val_t<token_id> val, Data data) {
                        if constexpr (is_empty_data<Data>) {
                            fn(val, data);
                        }
                    }(val<tokens>, token_data<tokens>),
                    ...);
            });
        }

        // For any `char` value, the `token_id` type that we should tokenize as.
        // E.g. '0' -> `token_id::int_lit`.
        //
        // Note that this is only the _first_ character of the token_id.
        constexpr std::array<token_id, 256> token_for_leading_char = [] {
            std::array<token_id, 256> result;
            for (token_id& t : result) {
                t = token_id::invalid;
            }

            const auto store = [&](unsigned char index, token_id token_id, bool force = false) {
                // We don't want collisions.
                assert(result[index] == token_id::invalid || force);
                result[index] = token_id;
            };

            for_each_empty_token([&]<token_id token_id, typename Data>(val_t<token_id>, Data) {
                store(static_cast<unsigned char>(Data::value[0]), token_id);
            });

            // token_id::space
            store(' ', token_id::space);
            store('\t', token_id::space);
            store('\n', token_id::space);
            store('\r', token_id::space);

            // token_id::ident
            for (unsigned char c = 'a'; c <= 'z'; ++c) {
                store(c, token_id::ident, /*force=*/true); // keyword collisions.
            }
            for (unsigned char c = 'A'; c <= 'Z'; ++c) {
                store(c, token_id::ident);
            }
            store('_', token_id::ident);

            // token_id::int_lit
            for (unsigned char c = '0'; c <= '9'; ++c) {
                store(c, token_id::int_lit);
            }

            // token_id::real_lit
            store('.', token_id::real_lit);

            return result;
        }();

        // A hash table to detemrine which identifiers are actually keywords.
        class keyword_table {
        private:
            static constexpr size_t num_keywords = [] {
                size_t count = 0;
                // Keywords are any empty token_id that had a collision with `ident`.
                for_each_empty_token([&]<token_id token_id, typename Data>(val_t<token_id>, Data) {
                    if (token_for_leading_char[static_cast<unsigned char>(Data::value[0])]
                        == token_id::ident) {
                        ++count;
                    }
                });
                return count;
            }();
            static constexpr size_t num_table_entries = 4;
            static_assert(num_table_entries >= num_keywords);

            // A perfect hash for the keywords.
            static constexpr uint8_t hash(std::string_view identifier) {
                // Arbitrarily chosen.
                return (static_cast<uint8_t>(identifier.front()) >> 3) & (num_table_entries - 1);
            }

        public:
            std::optional<token_id> find_keyword(std::string_view identifier) const {
                uint8_t hash_value = hash(identifier);
                entry entry = table_[hash_value];
                if (entry.value == identifier) {
                    return entry.token_id.value;
                }
                return std::nullopt;
            }

        private:
            struct entry {
                token_without_data token_id {val<noctern::token_id::empty_invalid>};
                std::string_view value;
            };

            std::array<entry, num_table_entries> table_ = [] {
                std::array<entry, num_table_entries> result;

                for_each_empty_token([&]<token_id token_id, typename Data>(val_t<token_id>, Data) {
                    if (token_for_leading_char[static_cast<unsigned char>(Data::value[0])]
                        == token_id::ident) {
                        uint8_t hash_value = hash(Data::value);
                        entry& entry = result[hash_value];
                        assert(entry.token_id.value == token_id::empty_invalid);
                        entry.value = Data::value;
                        entry.token_id = token_without_data(val<token_id>);
                    }
                });

                return result;
            }();
        };

        constexpr keyword_table keywords;

        struct tokenized_result {
            token_id id;
            token_index_t length;
        };

        // `tokenize_at` gets the next token_id where `token_for_leading_char` tells us which
        // overload to call. It is guaranteed that `input`'s first character matches the entry in
        // that table.
        //
        // We mutate the `input` in-out param to indicate that we've consumed input.
        //
        // We add the token_id (and string value if relevant) to the `builder` parameter.

        template <token_id token_id>
            requires is_empty_data<token_data_t<token_id>>
        tokenized_result tokenize_at(val_t<token_id>, std::string_view input) {
            // Possible optimization: it may be faster to generate a table rather than generate N
            // overloads.

            constexpr auto value = token_data_t<token_id>::value;
            if constexpr (value.size() == 1) {
                return {token_id, 1};
            } else {
                std::string_view data = value;

                if (input.size() < data.size()) {
                    return {token_id::invalid, static_cast<token_index_t>(input.size())};
                }

                return {token_id, static_cast<token_index_t>(data.size())};
            }
        }

        // Collects the front part of `input` which matches `p`.
        //
        // Assumes the first character matches `p`.
        template <typename Pred>
        token_index_t parse_while(std::string_view input, Pred&& p) {
            assert(!input.empty());
            assert(p(input[0]));

            auto last_valid = std::ranges::find_if_not(input.substr(1), p);
            int len = last_valid - input.begin();

            return len;
        }

        tokenized_result tokenize_at(val_t<token_id::invalid> token_id, std::string_view input) {
            return {token_id, parse_while(input, [](char c) {
                        return token_for_leading_char[static_cast<unsigned char>(c)]
                            == token_id::invalid;
                    })};
        }

        tokenized_result tokenize_at(val_t<token_id::space> token_id, std::string_view input) {
            return {token_id, parse_while(input, [](char c) {
                        return token_for_leading_char[static_cast<unsigned char>(c)]
                            == token_id::space;
                    })};
        }

        tokenized_result tokenize_at(val_t<token_id::ident>, std::string_view input) {
            token_index_t length = parse_while(input, [](char c) {
                return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
                    || c == '_';
            });
            if (std::optional<token_id> keyword = keywords.find_keyword(input.substr(0, length))) {
                return {*keyword, length};
            }

            return {token_id::ident, length};
        }

        // Tokenizes r'\.[0-9]*'.
        token_index_t tokenize_real_part_lit(std::string_view input) {
            std::string_view old_input = input;

            input.remove_prefix(1);
            auto last_valid
                = std::ranges::find_if_not(input, [](char c) { return '0' <= c && c <= '9'; });
            return last_valid - old_input.begin();
        }

        tokenized_result tokenize_at(val_t<token_id::real_lit> token_id, std::string_view input) {
            return {token_id, tokenize_real_part_lit(input)};
        }

        tokenized_result tokenize_at(val_t<token_id::int_lit>, std::string_view input) {
            token_index_t int_lit_length
                = parse_while(input, [](char c) { return '0' <= c && c <= '9'; });
            input.remove_prefix(int_lit_length);

            if (!input.empty()
                // We might actually need to combine this with a real number literal.
                && token_for_leading_char[static_cast<unsigned char>(input.front())]
                    == token_id::real_lit) {
                token_index_t real_lit_len = tokenize_real_part_lit(input);
                return {token_id::real_lit, int_lit_length + real_lit_len};
            } else {
                return {token_id::int_lit, int_lit_length};
            }
        }

        template <bool keep_spaces>
        tokens tokenize_all_impl(std::string_view input) {
            tokens::builder builder(input);

            while (!builder.remaining_input().empty()) {
                auto next = static_cast<unsigned char>(builder.remaining_input().front());
                tokenized_result token = enum_switch(
                    token_for_leading_char[next], [&]<token_id lex_next>(val_t<lex_next> val) {
                        return noctern::tokenize_at(val, builder.remaining_input());
                    });
                if (!keep_spaces && token.id == token_id::space) {
                    builder.add_ignored_token(token.length);
                } else {
                    builder.add_token(token.id, token.length);
                }
            }

            return tokens(std::move(builder));
        }
    }

    tokens tokenize_all(std::string_view input) {
        return noctern::tokenize_all_impl</*keep_spaces=*/false>(input);
    }

    tokens tokenize_all_keeping_spaces(std::string_view input) {
        return noctern::tokenize_all_impl</*keep_spaces=*/true>(input);
    }
}