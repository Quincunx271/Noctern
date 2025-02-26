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
                // the `token_id` values.
                start_marker = enum_max(type<token_id>),
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
        static_assert(enum_min(type<rule>) > enum_max(type<token_id>));

        class stack_op {
            static constexpr int max_token = enum_max(type<noctern::token_id>);
            static constexpr int max_rule = enum_max(type<noctern::rule>);

        public:
            explicit constexpr stack_op(noctern::token_id token_id)
                : value_(noctern::to_underlying(token_id)) {
            }

            explicit constexpr stack_op(noctern::rule rule)
                : value_(noctern::to_underlying(rule)) {
            }

            // The token_id, else `token_id::empty_invalid` if it's not a token_id.
            noctern::token_id token_id() const {
                if (int {value_} <= max_token) {
                    return static_cast<noctern::token_id>(value_);
                } else {
                    return token_id::empty_invalid;
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
                if (auto token_id = op.token_id(); token_id != token_id::empty_invalid) {
                    return std::string(stringify(token_id));
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
            explicit constexpr token_or_eof(noctern::token_id token_id)
                : token_(token_id) {
            }

            // The token_id, else `token_id::empty_invalid` if it's EOF.
            noctern::token_id token_id() const {
                return token_;
            }

            friend bool operator==(token_or_eof lhs, token_or_eof rhs) {
                return lhs.token_ == rhs.token_;
            }

            friend bool operator==(token_or_eof lhs, noctern::token_id rhs) {
                return lhs.token_ == rhs;
            }

            friend bool operator==(noctern::token_id lhs, token_or_eof rhs) {
                return rhs == lhs;
            }

        private:
            noctern::token_id token_;
        };

        constexpr token_or_eof token_eof {token_id::empty_invalid};
    }

    class parse_tree::builder {
        friend class parse_tree;

    public:
        void reserve(size_t size_hint) {
            postorder_.reserve(size_hint);
        }

        std::vector<token_id> postorder_;
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

            auto advance_token(token_id token_id) {
                if (tokens.empty() || tokens.front().token_id != token_id) {
                    assert(false && "parse error");
                }
                auto it = tokens.front();
                tokens.advance(1);
                return it;
            }

            auto push_token(tokens::const_iterator::token_and_str data) {
                builder.postorder_.push_back(data.token_id);
                builder.string_data_.push_back(data.string_data);
            }

            void parse_at(val_t<rule::file>) {
                while (!tokens.empty()) {
                    const auto token_and_str = tokens.front();
                    const token_id token_id = token_and_str.token_id;
                    if (token_id == token_id::fn_intro) {
                        parse_at(val<rule::fndef>);
                    } else {
                        // ERROR!
                        assert(false && "parse error");
                    }
                }
            }

            void parse_at(val_t<rule::fndef>) {
                builder.postorder_.push_back(advance_token(token_id::fn_intro).token_id);
                push_token(advance_token(token_id::ident));
                advance_token(token_id::lparen);

                parse_at(val<rule::fn_params>);

                builder.postorder_.push_back(advance_token(token_id::rparen).token_id);
                advance_token(token_id::fn_outro);

                parse_at(val<rule::expr>);

                builder.postorder_.push_back(advance_token(token_id::statement_end).token_id);
            }

            void parse_at(val_t<rule::fn_params>) {
                while (!tokens.empty() && tokens.front().token_id != token_id::rparen) {
                    push_token(advance_token(token_id::ident));

                    if (!tokens.empty() && tokens.front().token_id != token_id::rparen) {
                        if (tokens.front().token_id != token_id::comma) {
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
                token_id token_id = tokens.front().token_id;
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
                builder.postorder_.push_back(advance_token(token_id::lbrace).token_id);

                while (!tokens.empty() && tokens.front().token_id != token_id::return_) {
                    parse_at(val<rule::valdecl>);
                }

                parse_at(val<rule::return_>);

                builder.postorder_.push_back(advance_token(token_id::rbrace).token_id);
            }

            void parse_at(val_t<rule::return_>) {
                builder.postorder_.push_back(advance_token(token_id::return_).token_id);
                parse_at(val<rule::expr>);
                builder.postorder_.emplace_back(advance_token(token_id::statement_end).token_id);
            }

            void parse_at(val_t<rule::valdecl>) {
                builder.postorder_.push_back(advance_token(token_id::valdef_intro).token_id);
                push_token(advance_token(token_id::ident));
                advance_token(token_id::valdef_outro);

                parse_at(val<rule::expr>);

                builder.postorder_.push_back(advance_token(token_id::statement_end).token_id);
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
                token_id token_id = tokens.front().token_id;
                if (token_id == token_id::plus || token_id == token_id::minus) {
                    tokens.advance(1);
                    parse_at(val<rule::expr>);

                    builder.postorder_.push_back(token_id);
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
                token_id token_id = tokens.front().token_id;
                if (token_id == token_id::div || token_id == token_id::mult) {
                    tokens.advance(1);
                    parse_at(val<rule::div_mul_expr>);

                    builder.postorder_.push_back(token_id);
                }
            }

            void parse_at(val_t<rule::base_expr>) {
                if (tokens.empty()) {
                    // Error!
                    assert(false && "parse error");
                }
                token_id token_id = tokens.front().token_id;
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

    parse_tree parse(const tokens& input) {
        parser parser;
        parser.builder.reserve(input.num_tokens());
        parser.tokens = token_view(input.begin(), input.end());

        parser.parse_at(val<rule::file>);

        return parse_tree(std::move(parser.builder));
    }
}