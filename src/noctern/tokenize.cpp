#include "./tokenize.hpp"

#include <cassert>

namespace noctern {
    namespace {
        template <token token>
        using token_data_t = std::remove_cvref_t<decltype(token_data<token>)>;

        // This concept makes the static_assert print the name of the token with no defined data.
        template <token token>
        concept has_defined_token_data = !std::same_as<token_data_t<token>, std::nullptr_t>;

        static_assert(all_tokens([]<token... tokens>(val_t<tokens>...) {
            static_assert((has_defined_token_data<tokens> && ...));
            return true;
        }));

        template <typename Fn>
        constexpr void for_each_empty_token(Fn&& fn) {
            all_tokens([&]<token... tokens>(val_t<tokens>...) {
                (
                    [&]<token token, typename Data>(val_t<token> val, Data data) {
                        if constexpr (is_empty_data<Data>) {
                            fn(val, data);
                        }
                    }(val<tokens>, token_data<tokens>),
                    ...);
            });
        }

        constexpr std::array<token, 256> token_for_leading_char = [] {
            std::array<token, 256> result;
            for (token& t : result) {
                t = token::invalid;
            }

            const auto store = [&](unsigned char index, token token, bool force = false) {
                assert(result[index] == token::invalid || force);
                result[index] = token;
            };

            for_each_empty_token([&]<token token, typename Data>(val_t<token>, Data) {
                store(static_cast<unsigned char>(Data::value[0]), token);
            });

            // token::space
            store(' ', token::space);
            store('\t', token::space);
            store('\n', token::space);
            store('\r', token::space);

            // token::ident
            for (unsigned char c = 'a'; c <= 'z'; ++c) {
                store(c, token::ident, /*force=*/true); // keyword collisions.
            }
            for (unsigned char c = 'A'; c <= 'Z'; ++c) {
                store(c, token::ident);
            }
            store('_', token::ident);

            // token::int_lit
            for (unsigned char c = '0'; c <= '9'; ++c) {
                store(c, token::int_lit);
            }

            // token::real_lit
            store('.', token::real_lit);

            return result;
        }();

        class keyword_table {
        private:
            static constexpr size_t num_keywords = [] {
                size_t count = 0;
                for_each_empty_token([&]<token token, typename Data>(val_t<token>, Data) {
                    if (token_for_leading_char[static_cast<unsigned char>(Data::value[0])]
                        == token::ident) {
                        ++count;
                    }
                });
                return count;
            }();
            static constexpr size_t num_table_entries = 4;
            static_assert(num_table_entries >= num_keywords);

            static constexpr uint8_t hash(std::string_view identifier) {
                return (static_cast<uint8_t>(identifier[0]) >> 3) & (num_table_entries - 1);
            }

        public:
            std::optional<token> find_keyword(std::string_view identifier) const {
                uint8_t hash_value = hash(identifier);
                entry entry = table_[hash_value];
                if (entry.value == identifier) {
                    return entry.token;
                }
                return std::nullopt;
            }

        private:
            struct entry {
                noctern::token token = noctern::token::invalid;
                std::string_view value;
            };

            std::array<entry, num_table_entries> table_ = [] {
                std::array<entry, num_table_entries> result;

                for_each_empty_token([&]<token token, typename Data>(val_t<token>, Data) {
                    if (token_for_leading_char[static_cast<unsigned char>(Data::value[0])]
                        == token::ident) {
                        uint8_t hash_value = hash(Data::value);
                        entry& entry = result[hash_value];
                        assert(entry.token == token::invalid);
                        entry.value = Data::value;
                        entry.token = token;
                    }
                });

                return result;
            }();
        };

        constexpr keyword_table keywords;

        template <token token>
            requires is_empty_data<token_data_t<token>>
        noctern::token tokenize_at(
            val_t<token>, std::string_view& input, std::vector<std::string_view>& string_data) {
            constexpr auto value = token_data_t<token>::value;
            if constexpr (value.size() == 1) {
                input.remove_prefix(1);
                return token;
            } else {
                std::string_view data = value;

                if (input.size() < data.size()) {
                    string_data.push_back(input);
                    input.remove_prefix(input.size());
                    return token::invalid;
                }

                input.remove_prefix(data.size());
                return token;
            }
        }

        // Collects the front part of `input` which matches `p`.
        //
        // Assumes the first character matches `p`.
        template <typename Pred>
        std::string_view parse_while(std::string_view& input, Pred&& p) {
            assert(!input.empty());
            assert(p(input[0]));

            auto last_valid = std::ranges::find_if_not(input.substr(1), p);
            int len = last_valid - input.begin();

            std::string_view result = input.substr(0, len);
            input.remove_prefix(len);
            return result;
        }

        token tokenize_at(val_t<token::invalid>, std::string_view& input,
            std::vector<std::string_view>& string_data) {
            string_data.push_back(parse_while(input, [](char c) {
                return token_for_leading_char[static_cast<unsigned char>(c)] == token::invalid;
            }));

            return token::invalid;
        }

        token tokenize_at(val_t<token::space>, std::string_view& input,
            std::vector<std::string_view>& string_data) {
            string_data.push_back(parse_while(input, [](char c) {
                return token_for_leading_char[static_cast<unsigned char>(c)] == token::space;
            }));

            return token::space;
        }

        token tokenize_at(val_t<token::ident>, std::string_view& input,
            std::vector<std::string_view>& string_data) {
            std::string_view ident = parse_while(input, [](char c) {
                return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
                    || c == '_';
            });
            if (std::optional<token> keyword = keywords.find_keyword(ident)) {
                return *keyword;
            }
            string_data.push_back(ident);
            return token::ident;
        }

        std::string_view tokenize_real_part_lit(std::string_view& input) {
            std::string_view old_input = input;

            input.remove_prefix(1);
            auto last_valid
                = std::ranges::find_if_not(input, [](char c) { return '0' <= c && c <= '9'; });
            int len = last_valid - input.begin();
            input.remove_prefix(len);

            return old_input.substr(0, len + 1);
        }

        token tokenize_at(val_t<token::real_lit>, std::string_view& input,
            std::vector<std::string_view>& string_data) {
            string_data.push_back(tokenize_real_part_lit(input));

            return token::real_lit;
        }

        token tokenize_at(val_t<token::int_lit>, std::string_view& input,
            std::vector<std::string_view>& string_data) {
            std::string_view int_lit
                = parse_while(input, [](char c) { return '0' <= c && c <= '9'; });
            ;
            if (!input.empty()
                && token_for_leading_char[static_cast<unsigned char>(input[0])]
                    == token::real_lit) {
                std::string_view real_lit = tokenize_real_part_lit(input);
                string_data.emplace_back(int_lit.data(), &real_lit.back() + 1);
                return token::real_lit;
            } else {
                string_data.push_back(int_lit);
                return token::int_lit;
            }
        }
    }

    tokens tokenize_all(std::string_view input) {
        tokens result;

        while (!input.empty()) {
            auto next = static_cast<unsigned char>(input[0]);
            result.tokens_.push_back(token_switch(
                token_for_leading_char[next], [&]<token lex_next>(val_t<lex_next> val) {
                    return noctern::tokenize_at(val, input, result.string_data_);
                }));
        }

        return result;
    }
}