#include "./parser.hpp"

#include <catch2/catch.hpp>

#include "noctern/tokenize.test.hpp"

namespace noctern {
    namespace {
        struct elaborated_token {
            noctern::token_id token_id;
            std::string_view value;

            elaborated_token(noctern::token_id token_id)
                : token_id(token_id) {
                assert(!has_data(token_id));
            }
            elaborated_token(noctern::token_id token_id, std::string_view value)
                : token_id(token_id)
                , value(value) {
                assert(has_data(token_id));
            }
        };

        template <int N>
        void add_all(tokens::builder& builder, elaborated_token (&&tokens)[N]) {
            for (int i = 0; i < N; ++i) {
                enum_switch(tokens[i].token_id, [&]<token_id token_id>(val_t<token_id> t) {
                    if constexpr (has_data(token_id)) {
                        builder.add_token(t, tokens[i].value);
                    } else {
                        builder.add_token(t);
                    }
                });
            }
        }

        TEST_CASE("parse works") {
            tokens::builder tokens_builder;
            {
                using enum token_id;
                noctern::add_all(tokens_builder,
                    // def silly_add(x, y,): {
                    //     let z = y - 0.2;
                    //     return y + z  + x * 2. - 2 + .1;
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
                        {int_lit, "2"},
                        plus,
                        {real_lit, ".1"},
                        statement_end,
                        rbrace,
                        statement_end,
                    });
            }

            noctern::tokens tokens(std::move(tokens_builder));

            noctern::parse_tree result = noctern::parse(tokens);

            using enum noctern::token_id;
            CHECK_THAT(result.tokens(),
                Catch::Matchers::Equals(std::vector({
                    fn_intro,
                    ident,
                    ident,
                    ident,
                    rparen,
                    lbrace,
                    valdef_intro,
                    ident,
                    ident,
                    real_lit,
                    minus,
                    statement_end,
                    return_,
                    ident,
                    ident,
                    ident,
                    real_lit,
                    mult,
                    int_lit,
                    real_lit,
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
