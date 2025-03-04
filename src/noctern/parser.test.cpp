#include "./parser.hpp"

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

            friend bool operator==(const elaborated_token& lhs, const elaborated_token& rhs)
                = default;

            friend std::ostream& operator<<(std::ostream& out, const elaborated_token& token) {
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

        std::vector<elaborated_token> elaborate(const noctern::tokens& tokens) {
            std::vector<elaborated_token> result;

            for (const token token : tokens) {
                if (has_data(tokens.id(token))) {
                    result.emplace_back(tokens.id(token), std::string(tokens.string(token)));
                } else {
                    result.emplace_back(tokens.id(token));
                }
            }

            return result;
        }

        TEST_CASE("parse works") {
            using enum noctern::token_id;

            fabricated_tokens tokens = noctern::make_tokens(
                // def silly_add(x, y,): {
                //     let z = y - 0.2;
                //     return y + z  + x * 2. - return_me(2) + .1;
                // };
                {
                    fn_intro,
                    {ident, "silly_add"},
                    lparen,
                    {ident, "x"},
                    comma,
                    {ident, "y"},
                    comma,
                    rparen,
                    fn_outro,
                    lbrace,
                    valdef_intro,
                    {ident, "z"},
                    valdef_outro,
                    {ident, "y"},
                    minus,
                    {real_lit, "0.2"},
                    statement_end,
                    return_,
                    {ident, "y"},
                    plus,
                    {ident, "z"},
                    plus,
                    {ident, "x"},
                    mult,
                    {real_lit, "2."},
                    minus,
                    {ident, "return_me"},
                    lparen,
                    {int_lit, "2"},
                    rparen,
                    plus,
                    {real_lit, ".1"},
                    statement_end,
                    rbrace,
                    statement_end,
                });

            noctern::tokens result = noctern::parse(tokens.tokens);

            CHECK_THAT(noctern::elaborate(result),
                Catch::Matchers::Equals(std::vector<elaborated_token>({
                    fn_intro,
                    {ident, "silly_add"},
                    {ident, "x"},
                    {ident, "y"},
                    rparen,
                    lbrace,
                    valdef_intro,
                    {ident, "z"},
                    {ident, "y"},
                    {real_lit, "0.2"},
                    minus,
                    statement_end,
                    return_,
                    {ident, "y"},
                    {ident, "z"},
                    {ident, "x"},
                    {real_lit, "2."},
                    mult,
                    {ident, "return_me"},
                    lparen,
                    {int_lit, "2"},
                    rparen,
                    {real_lit, ".1"},
                    plus,
                    minus,
                    plus,
                    plus,
                    statement_end,
                    rbrace,
                    statement_end,
                })));
        }
    }
}
