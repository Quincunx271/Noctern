#include "./parser.hpp"

#include <algorithm>
#include <fmt/ostream.h>
#include <iostream>
#include <ostream>

namespace noctern {
    namespace {
        constexpr int max_token = all_tokens([]<token... tokens>(val_t<tokens>...) {
            return std::max({static_cast<int>(tokens)...});
        });

        enum class rule : uint8_t {
            // Ignored enum value. This is used to keep the `rule` enumeration values separate from
            // the `token` values.
            start_marker = max_token,
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

        template <typename Fn>
        constexpr decltype(auto) all_rules(Fn&& fn) {
            using enum rule;
            return std::invoke(std::forward<Fn>(fn)
#define NOCTERN_RULE_TYPE(name) , val<name>
                    NOCTERN_X_RULE(NOCTERN_RULE_TYPE)
#undef NOCTERN_RULE_TYPE
            );
        }

        constexpr std::string_view stringify(rule rule) {
            switch (rule) {
#define NOCTERN_RULE_STR(name)                                                                     \
    case rule::name: return #name;
                NOCTERN_X_RULE(NOCTERN_RULE_STR)
#undef NOCTERN_RULE_STR
            // these cases should never happen.
            case rule::start_marker: assert(false);
            case rule::empty_invalid: assert(false);
            }
            assert(false);
        }

        template <typename Fn>
        constexpr decltype(auto) rule_switch(rule rule, Fn&& fn) {
            switch (rule) {
#define NOCTERN_RULE_CASE(name)                                                                    \
    case rule::name: return std::invoke(std::forward<Fn>(fn), val<rule::name>);
                NOCTERN_X_RULE(NOCTERN_RULE_CASE)
#undef NOCTERN_RULE_CASE
            // these cases should never happen.
            case rule::start_marker: assert(false);
            case rule::empty_invalid: assert(false);
            }
            assert(false);
        }

        // Ensure that all rules are distinct from tokens.
        static_assert(all_rules([]<rule... rules>(val_t<rules>...) {
            return ((static_cast<int>(rules) > max_token) && ...);
        }));

        constexpr int max_rule = all_rules(
            []<rule... rules>(val_t<rules>...) { return std::max({static_cast<int>(rules)...}); });

        enum class action : uint8_t {
            // Ignored enum value. This is used to keep the `action` enumeration values separate
            // from
            // the `rule` values.
            start_marker = max_rule,
#define NOCTERN_X_ACTION(X)                                                                        \
    X(push_token)                                                                                  \
    X(push_token_and_pop)
#define NOCTERN_MAKE_ENUM_VALUE(name) name,
            NOCTERN_X_ACTION(NOCTERN_MAKE_ENUM_VALUE)
#undef NOCTERN_MAKE_ENUM_VALUE

            // A sentinel value.
            empty_invalid,
        };

        template <typename Fn>
        constexpr decltype(auto) all_actions(Fn&& fn) {
            using enum action;
            return std::invoke(std::forward<Fn>(fn)
#define NOCTERN_ACTION_TYPE(name) , val<name>
                    NOCTERN_X_ACTION(NOCTERN_ACTION_TYPE)
#undef NOCTERN_ACTION_TYPE
            );
        }

        constexpr std::string_view stringify(action action) {
            switch (action) {
#define NOCTERN_ACTION_STR(name)                                                                   \
    case action::name: return #name;
                NOCTERN_X_ACTION(NOCTERN_ACTION_STR)
#undef NOCTERN_ACTION_STR
            // these cases should never happen.
            case action::start_marker: assert(false);
            case action::empty_invalid: assert(false);
            }
            assert(false);
        }

        template <typename Fn>
        constexpr decltype(auto) action_switch(action action, Fn&& fn) {
            switch (action) {
#define NOCTERN_ACTION_CASE(name)                                                                  \
    case action::name: return std::invoke(std::forward<Fn>(fn), val<action::name>);
                NOCTERN_X_ACTION(NOCTERN_ACTION_CASE)
#undef NOCTERN_ACTION_CASE
            // These cases should never happen.
            case action::start_marker: assert(false);
            case action::empty_invalid: assert(false);
            }
            assert(false);
        }

        // Ensure that all actions are distinct from rules.
        static_assert(all_actions([]<action... actions>(val_t<actions>...) {
            return ((static_cast<int>(actions) > max_rule) && ...);
        }));
        // Ensure that all actions are distinct from tokens.
        static_assert(all_actions([]<action... actions>(val_t<actions>...) {
            return ((static_cast<int>(actions) > max_token) && ...);
        }));

        class stack_op {
        public:
            explicit constexpr stack_op(noctern::token token)
                : value_(static_cast<uint8_t>(token)) {
            }

            explicit constexpr stack_op(noctern::rule rule)
                : value_(static_cast<uint8_t>(rule)) {
            }

            explicit constexpr stack_op(noctern::action action)
                : value_(static_cast<uint8_t>(action)) {
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

            // The action, else `action::empty_invalid` if it's not a action.
            noctern::action action() const {
                if (int {value_} > max_rule) {
                    return static_cast<noctern::action>(value_);
                } else {
                    return action::empty_invalid;
                }
            }

            friend std::string format_as(stack_op op) {
                if (auto token = op.token(); token != token::empty_invalid) {
                    return std::string(stringify(token));
                } else if (auto rule = op.rule(); rule != rule::empty_invalid) {
                    return std::string(stringify(rule));
                } else if (auto action = op.action(); action != action::empty_invalid) {
                    return std::string(stringify(op.action()));
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

        // Mental model: parsing is actually a stack machine interpreter, with `stack_` being the
        // bytecode. rules and tokens are a bytecode which either pops tokens (if they match), or
        // expands out to more actions (rules do this). Maybe I can implement error handling and
        // even pulling out semantic values as custom actions. Or maybe I can automate pulling out
        // semantic values.
        std::vector<stack_op> stack_;
        std::vector<token> postorder_;
        std::vector<std::string_view> string_data_;
    };

    parse_tree::parse_tree(builder builder)
        : postorder_(std::move(builder.postorder_)) {
    }

    namespace {
        template <rule rule>
        void finish_rule(val_t<rule>, parse_tree::builder&) {
        }

        void parse_at(val_t<rule::file>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::fn_intro) {
                builder.stack_.emplace_back(rule::file);
                builder.stack_.emplace_back(rule::fndef);
            } else if (token == token_eof) {
                // builder.stack_.emplace_back(token_eof);
                return;
            } else {
                // ERROR!
                assert(false && "parse error");
            }
        }

        void parse_at(val_t<rule::fndef>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token != token::fn_intro) {
                // ERROR! Or maybe just return?
                assert(false && "parse error");
            }
            builder.stack_.emplace_back(token::statement_end);
            builder.stack_.emplace_back(action::push_token);
            builder.stack_.emplace_back(rule::expr);
            builder.stack_.emplace_back(token::fn_outro);
            builder.stack_.emplace_back(token::rparen);
            builder.stack_.emplace_back(rule::fn_params);
            builder.stack_.emplace_back(token::lparen);
            builder.stack_.emplace_back(token::ident);
            builder.stack_.emplace_back(token::fn_intro);

            builder.postorder_.push_back(token::fn_intro);
        }

        void parse_at(val_t<rule::fn_params>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::rparen) {
                builder.postorder_.emplace_back(token::rparen);
                return;
            }
            if (token != token::ident) {
                assert(false && "parse error");
            }

            builder.stack_.emplace_back(rule::fn_params);
            builder.stack_.emplace_back(token::comma);
            builder.stack_.emplace_back(token::ident);
        }

        void finish_rule(val_t<rule::fn_params>, parse_tree::builder& builder) {
            builder.postorder_.push_back(token::rparen);
        }

        void parse_at(val_t<rule::expr>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::lbrace) {
                builder.stack_.emplace_back(rule::block);
            } else if (token == token::int_lit || token == token::real_lit || token == token::lparen
                || token == token::ident) {
                builder.stack_.emplace_back(rule::add_sub_expr);
            } else {
                // ERROR! Or maybe just return?
                assert(false && "parse error");
            }
        }

        void parse_at(val_t<rule::block>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            builder.stack_.emplace_back(token::rbrace);
            builder.stack_.emplace_back(action::push_token);
            builder.stack_.emplace_back(rule::return_);
            builder.stack_.emplace_back(rule::valdecl);
            builder.stack_.emplace_back(token::lbrace);

            builder.postorder_.push_back(token::lbrace);
        }

        void parse_at(val_t<rule::return_>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            builder.stack_.emplace_back(token::statement_end);
            builder.stack_.emplace_back(action::push_token);
            builder.stack_.emplace_back(rule::expr);
            builder.stack_.emplace_back(token::return_);

            builder.postorder_.emplace_back(token::return_);
        }

        void parse_at(val_t<rule::valdecl>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            builder.stack_.emplace_back(token::statement_end);
            builder.stack_.emplace_back(action::push_token);
            builder.stack_.emplace_back(rule::expr);
            builder.stack_.emplace_back(token::valdef_outro);
            builder.stack_.emplace_back(token::ident);
            builder.stack_.emplace_back(token::valdef_intro);

            builder.postorder_.push_back(token::valdef_intro);
        }

        void parse_at(val_t<rule::add_sub_expr>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            builder.stack_.emplace_back(rule::add_sub_expr2);
            builder.stack_.emplace_back(rule::div_mul_expr);
        }

        void parse_at(val_t<rule::add_sub_expr2>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::plus || token == token::minus) {
                builder.stack_.emplace_back(token.token());
                builder.stack_.emplace_back(action::push_token_and_pop);

                builder.stack_.emplace_back(rule::expr);
                builder.stack_.emplace_back(token.token());
            }
        }

        void parse_at(val_t<rule::div_mul_expr>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            builder.stack_.emplace_back(rule::div_mul_expr2);
            builder.stack_.emplace_back(rule::base_expr);
        }

        void parse_at(val_t<rule::div_mul_expr2>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::div || token == token::mult) {
                builder.stack_.emplace_back(token.token());
                builder.stack_.emplace_back(action::push_token_and_pop);

                builder.stack_.emplace_back(rule::div_mul_expr);
                builder.stack_.emplace_back(token.token());
            }
        }

        void parse_at(val_t<rule::base_expr>, token_or_eof token, std::string_view token_value,
            parse_tree::builder& builder) {
            if (token == token::lparen) {
                builder.stack_.emplace_back(token::rparen);
                builder.stack_.emplace_back(rule::expr);
                builder.stack_.emplace_back(token::lparen);
            } else if (token == token::int_lit) {
                builder.stack_.emplace_back(token::int_lit);
            } else if (token == token::real_lit) {
                builder.stack_.emplace_back(token::real_lit);
            } else if (token == token::ident) {
                builder.stack_.emplace_back(token::ident);
            } else {
                // ERROR
                assert(false && "parse error");
            }
        }
    }

    parse_tree parse(const tokens& input) {
        {
            fmt::println("Tokens {}:", input.num_tokens());
            input.walk([](token token, std::string_view value) {
                if (has_data(token)) {
                    fmt::println("\t{}: {}", stringify(token), value);
                } else {
                    fmt::println("\t{}", stringify(token));
                }
            });
        }
        parse_tree::builder builder;
        builder.reserve(input.num_tokens());
        builder.stack_.push_back(stack_op(rule::file));

        input.walk([&](noctern::token token, std::string_view value) {
            if (token == token::space) {
                return;
            }
            fmt::println("Saw {}", stringify(token));
            while (true) {
                fmt::println(std::cerr, "Stack {}: {}", builder.stack_.size(),
                    fmt::join(builder.stack_, ","));
                stack_op next = builder.stack_.back();
                builder.stack_.pop_back();

                if (auto next_action = next.action(); next_action != action::empty_invalid) {
                    assert(next_action == action::push_token
                        || next_action == action::push_token_and_pop);

                    stack_op token = builder.stack_.back();
                    assert(token.token() != token::empty_invalid);
                    builder.postorder_.push_back(token.token());
                    if (next_action == action::push_token_and_pop) {
                        builder.stack_.pop_back();
                    }
                    continue;
                } else if (auto next_token = next.token(); next_token != token::empty_invalid) {
                    if (token != next_token) {
                        fmt::println(
                            std::cerr, "Parse error; got: {} want: {}", stringify(token), next);
                        fmt::println(std::cerr, "Stack {}: {}", builder.stack_.size(),
                            fmt::join(builder.stack_, ","));
                        // Parse error!
                        assert(false && "parse error");
                    } else {
                        if (has_data(token)) {
                            builder.postorder_.push_back(next_token);
                            builder.string_data_.push_back(value);
                        }
                        return;
                    }
                }
                assert(next.rule() != rule::empty_invalid);
                rule_switch(next.rule(), [&]<rule rule>(val_t<rule> top_rule) {
                    parse_at(top_rule, token_or_eof(token), value, builder);
                });
            }
        });
        while (!builder.stack_.empty()) {
            stack_op next = builder.stack_.back();
            builder.stack_.pop_back();

            if (auto next_token = next.token(); next_token != token::empty_invalid) {
                // Parse error! Not eof.
                assert(false && "parse error");
            }
            assert(next.rule() != rule::empty_invalid);
            rule_switch(next.rule(), [&]<rule rule>(val_t<rule> top_rule) {
                parse_at(top_rule, token_eof, "", builder);
            });
        }
        fmt::println("Tokens {}:", builder.postorder_.size());
        int index = 0;
        for (token token : builder.postorder_) {
            if (has_data(token)) {
                fmt::println("\t{}: {}", stringify(token), builder.string_data_[index++]);
            } else {
                fmt::println("\t{}", stringify(token));
            }
        }

        return parse_tree(std::move(builder));
    }

}