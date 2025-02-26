#include "./parser.hpp"

#include <catch2/catch.hpp>

namespace noctern {
    namespace {
        struct elaborated_token {
            noctern::token token;
            std::string_view value;

            elaborated_token(noctern::token token)
                : token(token) {
                assert(!has_data(token));
            }
            elaborated_token(noctern::token token, std::string_view value)
                : token(token)
                , value(value) {
                assert(has_data(token));
            }
        };

        template <int N>
        void add_all(tokens::builder& builder, elaborated_token (&&tokens)[N]) {
            for (int i = 0; i < N; ++i) {
                enum_switch(tokens[i].token, [&]<token token>(val_t<token> t) {
                    if constexpr (has_data(token)) {
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
                using enum token;
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
        }
    }
}
