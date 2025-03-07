#include "./interpreter.hpp"

#include <catch2/catch.hpp>
#include <ostream>
#include <vector>

#include "noctern/tokenize.test.hpp"

namespace noctern {
    namespace {
        struct elaborated_token {
            noctern::token_id token_id;
            std::string value;

            elaborated_token(noctern::token_id token_id)
                : token_id(token_id) {
                assert(!has_data(token_id));
            }
            elaborated_token(noctern::token_id token_id, std::string value)
                : token_id(token_id)
                , value(std::move(value)) {
                assert(has_data(token_id));
            }

            [[maybe_unused]] // Silence unused warnings.
            friend bool
            operator==(const elaborated_token& lhs, const elaborated_token& rhs)
                = default;

            [[maybe_unused]] // Silence unused warnings.
            friend std::ostream&
            operator<<(std::ostream& out, const elaborated_token& token) {
                out << "<" << stringify(token.token_id);
                if (!token.value.empty()) {
                    out << ": " << token.value;
                }
                return out << ">";
            }
        };

        struct fabricated_tokens {
            std::vector<char> data;
            noctern::tokens tokens;
        };

        template <int N>
        fabricated_tokens make_tokens(elaborated_token (&&tokens)[N]) {
            std::vector<char> data;
            for (const elaborated_token& token : tokens) {
                data.insert(data.end(), token.value.begin(), token.value.end());
            }

            tokens::builder builder(std::string_view(data.data(), data.size()));

            for (const elaborated_token& token : tokens) {
                builder.add_token(token.token_id, token.value.size());
            }

            return fabricated_tokens {
                std::move(data),
                noctern::tokens(std::move(builder)),
            };
        }

        TEST_CASE("interpreter works") {
            using enum noctern::token_id;

            fabricated_tokens tokens = noctern::make_tokens(
                // def silly_add(x, y,): {
                //     let z = y - 0.2;
                //     return y + z  + x * 2. - 2 + .1;
                // };
                {
                    def,
                    {ident, "silly_add"},
                    {ident, "x"},
                    {ident, "y"},
                    rparen,
                    lbrace,
                    let,
                    {ident, "z"},
                    {ident, "y"},
                    {real_lit, "0.2"},
                    minus,
                    semicolon,
                    return_,
                    {ident, "y"},
                    {ident, "z"},
                    plus,
                    {ident, "x"},
                    {real_lit, "2."},
                    mult,
                    plus,
                    {int_lit, "2"},
                    minus,
                    {real_lit, ".1"},
                    plus,
                    semicolon,
                    rbrace,
                    semicolon,
                });

            noctern::compilation_unit cu(tokens.tokens);
            noctern::string_intern_table global_symbols;
            noctern::symbol_table st(tokens.tokens, cu, global_symbols);
            noctern::interpreter interpreter(st);

            // TODO: safely unwrap this.
            noctern::token silly_add = *st.find_fn_decl(global_symbols.intern("silly_add"));

            double x = 42.3;
            double y = -2.9;
            CHECK(interpreter.eval_fn(tokens.tokens, silly_add, noctern::interpreter::frame{
                .locals = {
                    {"x", x},
                    {"y", y},
                },
                .expr_stack = {},
            }) == y + (y - 0.2) + x * 2. - 2 + .1);
        }
    }
}
