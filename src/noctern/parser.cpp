#include "./parser.hpp"

#include <algorithm>

#include "noctern/enum.hpp"
#include "noctern/meta.hpp"

namespace noctern {
    namespace {
        struct _rule_wrapper {
            enum class rule : uint8_t {
                // Ignored enum value. This is used to keep the `rule` enumeration values separate
                // from
                // the `token` values.
                start_marker = enum_max(type<token>),
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
    X(div_mul_expr) /*  ::= base_expr div_mul_expr2 */                                             \
    X(div_mul_expr2) /*  ::= </> div_mul_expr | <*> div_mul_expr | */                              \
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
                case rule::start_marker: assert(false);
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

        // Ensure that all rules are distinct from tokens.
        static_assert(enum_min(type<rule>) > enum_max(type<token>));

        class stack_op {
            static constexpr int max_token = enum_max(type<noctern::token>);
            static constexpr int max_rule = enum_max(type<noctern::rule>);

        public:
            explicit constexpr stack_op(noctern::token token)
                : value_(noctern::to_underlying(token)) {
            }

            explicit constexpr stack_op(noctern::rule rule)
                : value_(noctern::to_underlying(rule)) {
            }

            // The token, else `token::empty_invalid` if it's not a token.
            noctern::token token() const {
                if (int {value_} <= max_token) {
                    return static_cast<noctern::token>(value_);
                } else {
                    return token::empty_invalid;
                }
            }

            // The rule, else `rule::empty_invalid` if it's not a rule.
            noctern::rule rule() const {
                if (max_token < int {value_} && int {value_} <= max_rule) {
                    return static_cast<noctern::rule>(value_);
                } else {
                    return rule::empty_invalid;
                }
            }

            friend std::string format_as(stack_op op) {
                if (auto token = op.token(); token != token::empty_invalid) {
                    return std::string(stringify(token));
                } else if (auto rule = op.rule(); rule != rule::empty_invalid) {
                    return std::string(stringify(rule));
                }
                return std::to_string(int {op.value_});
            }

        private:
            uint8_t value_;
        };

        class token_or_eof {
        public:
            explicit constexpr token_or_eof(noctern::token token)
                : token_(token) {
            }

            // The token, else `token::empty_invalid` if it's EOF.
            noctern::token token() const {
                return token_;
            }

            friend bool operator==(token_or_eof lhs, token_or_eof rhs) {
                return lhs.token_ == rhs.token_;
            }

            friend bool operator==(token_or_eof lhs, noctern::token rhs) {
                return lhs.token_ == rhs;
            }

            friend bool operator==(noctern::token lhs, token_or_eof rhs) {
                return rhs == lhs;
            }

        private:
            noctern::token token_;
        };

        constexpr token_or_eof token_eof {token::empty_invalid};
    }

    class parse_tree::builder {
        friend class parse_tree;

    public:
        void reserve(size_t size_hint) {
            postorder_.reserve(size_hint);
        }

        std::vector<token> postorder_;
        std::vector<std::string_view> string_data_;
    };

    parse_tree::parse_tree(builder builder)
        : postorder_(std::move(builder.postorder_)) {
    }

    namespace {
        using token_view = std::ranges::subrange<tokens::const_iterator>;

        struct parser {
            token_view tokens;
            parse_tree::builder builder;

            auto advance_token(token token) {
                if (tokens.empty() || tokens.front().token != token) {
                    assert(false && "parse error");
                }
                auto it = tokens.front();
                tokens.advance(1);
                return it;
            }

            auto push_token(tokens::const_iterator::token_and_str data) {
                builder.postorder_.push_back(data.token);
                builder.string_data_.push_back(data.string_data);
            }

            void parse_at(val_t<rule::file>) {
                while (!tokens.empty()) {
                    const auto token_and_str = tokens.front();
                    const token token = token_and_str.token;
                    if (token == token::fn_intro) {
                        parse_at(val<rule::fndef>);
                    } else {
                        // ERROR!
                        assert(false && "parse error");
                    }
                }
            }

            void parse_at(val_t<rule::fndef>) {
                builder.postorder_.push_back(advance_token(token::fn_intro).token);
                push_token(advance_token(token::ident));
                advance_token(token::lparen);

                parse_at(val<rule::fn_params>);

                builder.postorder_.push_back(advance_token(token::rparen).token);
                advance_token(token::fn_outro);

                parse_at(val<rule::expr>);

                builder.postorder_.push_back(advance_token(token::statement_end).token);
            }

            void parse_at(val_t<rule::fn_params>) {
                while (!tokens.empty() && tokens.front().token != token::rparen) {
                    push_token(advance_token(token::ident));

                    if (!tokens.empty() && tokens.front().token != token::rparen) {
                        if (tokens.front().token != token::comma) {
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
                token token = tokens.front().token;
                if (token == token::lbrace) {
                    parse_at(val<rule::block>);
                } else if (token == token::int_lit || token == token::real_lit
                    || token == token::lparen || token == token::ident) {
                    parse_at(val<rule::add_sub_expr>);
                } else {
                    // ERROR! Or maybe just return?
                    assert(false && "parse error");
                }
            }

            void parse_at(val_t<rule::block>) {
                builder.postorder_.push_back(advance_token(token::lbrace).token);

                while (!tokens.empty() && tokens.front().token != token::return_) {
                    parse_at(val<rule::valdecl>);
                }

                parse_at(val<rule::return_>);

                builder.postorder_.push_back(advance_token(token::rbrace).token);
            }

            void parse_at(val_t<rule::return_>) {
                builder.postorder_.push_back(advance_token(token::return_).token);
                parse_at(val<rule::expr>);
                builder.postorder_.emplace_back(advance_token(token::statement_end).token);
            }

            void parse_at(val_t<rule::valdecl>) {
                builder.postorder_.push_back(advance_token(token::valdef_intro).token);
                push_token(advance_token(token::ident));
                advance_token(token::valdef_outro);

                parse_at(val<rule::expr>);

                builder.postorder_.push_back(advance_token(token::statement_end).token);
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
                token token = tokens.front().token;
                if (token == token::plus || token == token::minus) {
                    tokens.advance(1);
                    parse_at(val<rule::expr>);

                    builder.postorder_.push_back(token);
                }
            }

            void parse_at(val_t<rule::div_mul_expr>) {
                parse_at(val<rule::base_expr>);
                parse_at(val<rule::div_mul_expr2>);
            }

            void parse_at(val_t<rule::div_mul_expr2>) {
                if (tokens.empty()) {
                    // Okay!
                    return;
                }
                token token = tokens.front().token;
                if (token == token::div || token == token::mult) {
                    tokens.advance(1);
                    parse_at(val<rule::div_mul_expr>);

                    builder.postorder_.push_back(token);
                }
            }

            void parse_at(val_t<rule::base_expr>) {
                if (tokens.empty()) {
                    // Error!
                    assert(false && "parse error");
                }
                token token = tokens.front().token;
                if (token == token::lparen) {
                    tokens.advance(1);
                    parse_at(val<rule::expr>);

                    advance_token(token::rparen);
                } else if (token == token::int_lit || token == token::real_lit
                    || token == token::ident) {
                    push_token(advance_token(token));
                } else {
                    // ERROR
                    assert(false && "parse error");
                }
            }
        };
    }

    parse_tree parse(const tokens& input) {
        parser parser;
        parser.builder.reserve(input.num_tokens());
        parser.tokens = token_view(input.begin(), input.end());

        parser.parse_at(val<rule::file>);

        return parse_tree(std::move(parser.builder));
    }
}