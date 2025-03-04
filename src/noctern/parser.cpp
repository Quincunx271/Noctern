#include "./parser.hpp"

#include <algorithm>

#include "noctern/enum.hpp"
#include "noctern/meta.hpp"

namespace noctern {
    namespace {
        struct _rule_wrapper {
            enum class rule : uint8_t {
#define NOCTERN_X_RULE(X)                                                                          \
    X(file) /*          ::= (list) fndef */                                                        \
    X(fndef) /*         ::= <fn_intro> <ident> <(> fn_params <)> <:> expr <;> */                   \
    X(fn_params) /*     ::= (list: join <,>) <ident> */                                            \
    X(expr) /*          ::= block | add_sub_expr */                                                \
    X(block) /*         ::= <{> ((list) valdecl) return_ <}> */                                    \
    X(return_) /*       ::= <return_> expr <;> */                                                  \
    X(valdecl) /*       ::= <valdef_intro> <ident> <=> expr <;> */                                 \
    X(add_sub_expr) /*  ::= div_mul_expr add_sub_expr2 */                                          \
    X(add_sub_expr2) /* ::=  <+> expr | <-> expr | */                                              \
    X(div_mul_expr) /*  ::= fn_call_expr div_mul_expr2 */                                          \
    X(div_mul_expr2) /* ::= </> div_mul_expr | <*> div_mul_expr | */                               \
    X(fn_call_expr) /*  ::= base_expr fn_call_expr2 */                                             \
    X(fn_call_expr2) /* ::= <(> (list: join <,>) expr <)> | */                                     \
    X(base_expr) /*     ::= <(> expr <)> | <int_lit> | <real_lit> | <ident> */
#define NOCTERN_MAKE_ENUM_VALUE(name) name,
                NOCTERN_X_RULE(NOCTERN_MAKE_ENUM_VALUE)
#undef NOCTERN_MAKE_ENUM_VALUE

                // A sentinel value which doesn't need to be handled because it doesn't occur.
                empty_invalid,
            };

        private:
            friend enum_mixin;

            template <typename Fn>
            friend constexpr decltype(auto) switch_introspect(rule rule, Fn&& fn) {
                switch (rule) {
#define NOCTERN_RULE_INTROSPECT(name)                                                              \
    case rule::name: {                                                                             \
        constexpr std::string_view name_str = #name;                                               \
        return std::invoke(std::forward<Fn>(fn), val<rule::name>, name_str);                       \
    }
                    NOCTERN_X_RULE(NOCTERN_RULE_INTROSPECT)
#undef NOCTERN_RULE_INTROSPECT
                case rule::empty_invalid: assert(false);
                }
                assert(false);
            }

            template <typename Fn>
            friend constexpr decltype(auto) introspect(type_t<rule>, Fn&& fn) {
                using enum rule;
                return std::invoke(std::forward<Fn>(fn)
#define NOCTERN_RULE_TYPE(name) , val<name>
                        NOCTERN_X_RULE(NOCTERN_RULE_TYPE)
#undef NOCTERN_RULE_TYPE
                );
            }
#undef NOCTERN_X_RULE
        };

        using rule = _rule_wrapper::rule;

        using token_view = std::ranges::subrange<tokens::const_iterator>;

        struct parser {
            noctern::tokens& input;
            token_view tokens;
            tokens::const_iterator out;

            auto advance_token(token_id token_id) {
                if (tokens.empty()) {
                    assert(false && "parse error");
                }
                if (input.id(tokens.front()) != token_id) {
                    assert(false && "parse error");
                }
                auto it = input.extract(tokens.begin());
                tokens.advance(1);
                return it;
            }

            auto push_token(tokens::extracted_data token) {
                assert(out <= tokens.begin());
                assert(token.id != token_id::invalid);
                input.store(out++, token);
            }

            void parse_at(val_t<rule::file>) {
                while (!tokens.empty()) {
                    const token_id token_id = input.id(tokens.front());
                    if (token_id == token_id::fn_intro) {
                        parse_at(val<rule::fndef>);
                    } else {
                        // ERROR!
                        assert(false && "parse error");
                    }
                }
            }

            void parse_at(val_t<rule::fndef>) {
                push_token(advance_token(token_id::fn_intro));
                push_token(advance_token(token_id::ident));
                advance_token(token_id::lparen);

                parse_at(val<rule::fn_params>);

                push_token(advance_token(token_id::rparen));
                advance_token(token_id::fn_outro);

                parse_at(val<rule::expr>);

                push_token(advance_token(token_id::statement_end));
            }

            void parse_at(val_t<rule::fn_params>) {
                while (!tokens.empty() && input.id(tokens.front()) != token_id::rparen) {
                    push_token(advance_token(token_id::ident));

                    if (!tokens.empty() && input.id(tokens.front()) != token_id::rparen) {
                        if (input.id(tokens.front()) != token_id::comma) {
                            assert(false && "expected comma");
                        }
                        tokens.advance(1);
                    }
                }
                if (tokens.empty()) {
                    assert(false && "parse error");
                }
            }

            void parse_at(val_t<rule::expr>) {
                if (tokens.empty()) {
                    // ERROR!
                    assert(false && "parse error");
                }
                token_id token_id = input.id(tokens.front());
                if (token_id == token_id::lbrace) {
                    parse_at(val<rule::block>);
                } else if (token_id == token_id::int_lit || token_id == token_id::real_lit
                    || token_id == token_id::lparen || token_id == token_id::ident) {
                    parse_at(val<rule::add_sub_expr>);
                } else {
                    // ERROR! Or maybe just return?
                    assert(false && "parse error");
                }
            }

            void parse_at(val_t<rule::block>) {
                push_token(advance_token(token_id::lbrace));

                while (!tokens.empty() && input.id(tokens.front()) != token_id::return_) {
                    parse_at(val<rule::valdecl>);
                }

                parse_at(val<rule::return_>);

                push_token(advance_token(token_id::rbrace));
            }

            void parse_at(val_t<rule::return_>) {
                push_token(advance_token(token_id::return_));
                parse_at(val<rule::expr>);
                push_token(advance_token(token_id::statement_end));
            }

            void parse_at(val_t<rule::valdecl>) {
                push_token(advance_token(token_id::valdef_intro));
                push_token(advance_token(token_id::ident));
                advance_token(token_id::valdef_outro);

                parse_at(val<rule::expr>);

                push_token(advance_token(token_id::statement_end));
            }

            void parse_at(val_t<rule::add_sub_expr>) {
                // TODO: avoid recursing here.
                parse_at(val<rule::div_mul_expr>);
                parse_at(val<rule::add_sub_expr2>);
            }

            void parse_at(val_t<rule::add_sub_expr2>) {
                if (tokens.empty()) {
                    // Okay!
                    return;
                }
                token_id token_id = input.id(tokens.front());
                if (token_id == token_id::plus || token_id == token_id::minus) {
                    auto token = input.extract(tokens.begin());
                    tokens.advance(1);
                    parse_at(val<rule::expr>);

                    push_token(token);
                }
            }

            void parse_at(val_t<rule::div_mul_expr>) {
                parse_at(val<rule::fn_call_expr>);
                parse_at(val<rule::div_mul_expr2>);
            }

            void parse_at(val_t<rule::div_mul_expr2>) {
                if (tokens.empty()) {
                    // Okay!
                    return;
                }
                token_id token_id = input.id(tokens.front());
                if (token_id == token_id::div || token_id == token_id::mult) {
                    auto token = input.extract(tokens.begin());
                    tokens.advance(1);
                    parse_at(val<rule::div_mul_expr>);

                    push_token(token);
                }
            }

            void parse_at(val_t<rule::fn_call_expr>) {
                parse_at(val<rule::base_expr>);
                parse_at(val<rule::fn_call_expr2>);
            }

            void parse_at(val_t<rule::fn_call_expr2>) {
                if (tokens.empty()) {
                    // Okay!
                    return;
                }
                token_id token_id = input.id(tokens.front());
                if (token_id == token_id::lparen) {
                    push_token(advance_token(token_id::lparen));

                    while (!tokens.empty() && input.id(tokens.front()) != token_id::rparen) {
                        parse_at(val<rule::expr>);

                        if (!tokens.empty() && input.id(tokens.front()) != token_id::rparen) {
                            advance_token(token_id::comma);
                        }
                    }

                    push_token(advance_token(token_id::rparen));
                }
            }

            void parse_at(val_t<rule::base_expr>) {
                if (tokens.empty()) {
                    // Error!
                    assert(false && "parse error");
                }
                token_id token_id = input.id(tokens.front());
                if (token_id == token_id::lparen) {
                    tokens.advance(1);
                    parse_at(val<rule::expr>);

                    advance_token(token_id::rparen);
                } else if (token_id == token_id::int_lit || token_id == token_id::real_lit
                    || token_id == token_id::ident) {
                    push_token(advance_token(token_id));
                } else {
                    // ERROR
                    assert(false && "parse error");
                }
            }
        };
    }

    tokens parse(tokens input) {
        parser parser {
            .input = input,
            .tokens = token_view(input.begin(), input.end()),
            .out = input.begin(),
        };

        parser.parse_at(val<rule::file>);

        for (tokens::const_iterator cpy = parser.out; cpy != input.end(); ++cpy) {
            assert(input.id(*cpy) == token_id::invalid);
        }
        input.erase_to_end(parser.out);

        return input;
    }
}