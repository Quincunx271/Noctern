#include "./parser.hpp"

#include <cassert>
#include <optional>
#include <utility>

namespace noctern {
    namespace {
        template <token_type TokenType>
        constexpr auto parse_literal = [](token const*& iter, token const* end) -> bool {
            if (iter == end) return false;
            if (iter->type != TokenType) return false;
            ++iter;
            return true;
        };

        template <token_type TokenType>
        constexpr auto parse_token
            = [](token const*& iter, token const* end) -> std::optional<token> {
            if (iter == end) return std::nullopt;
            if (iter->type != TokenType) return std::nullopt;
            auto tok = *iter;
            ++iter;
            return tok;
        };

        constexpr auto parse_identifier
            = [](token const*& iter, token const* end) -> std::optional<ast::identifier> {
            if (auto m_id = parse_token<token_type::identifier>(iter, end)) {
                return ast::identifier(std::string(m_id->text));
            }
            return std::nullopt;
        };

        auto parse_type(token const*& iter, token const* end) -> std::optional<ast::type> {
            if (iter == end) return std::nullopt;
            auto first = iter;
            // type ::= identifier | fn_type | eval_type
            // fn_type ::= type "->" type
            // eval_type ::= identifier "[" list(type, ",") "]"

            // type ::= identifier type'
            // type' ::= embellish trailing
            // embellish ::= eval_type' | Empty
            // eval_type' ::= "[" list(type, ",") "]"
            // trailing ::= fn_type' *
            // fn_type' ::= "->" type

            auto m_id = parse_identifier(first, end);
            if (!m_id) return std::nullopt;
            if (first == end) return ast::type(ast::basic_type(std::move(*m_id)));

            ast::type result(ast::basic_type(std::move(*m_id)));

            if (parse_literal<token_type::ltcall>(first, end)) {
                std::vector<ast::type> eval_args;

                while (auto m_type = parse_type(first, end)) {
                    eval_args.emplace_back(std::move(*m_type));
                    if (!parse_literal<token_type::list_sep>(first, end)) { break; }
                    if (first == end) return std::nullopt;
                }

                if (!parse_literal<token_type::rtcall>(first, end)) return std::nullopt;
                if (first == end) return std::nullopt;

                result = ast::type(ast::eval_type {
                    .base = std::move(std::get<ast::basic_type>(result.value)),
                    .args = std::move(eval_args),
                });
            }

            while (parse_literal<token_type::farrow>(first, end)) {
                auto m_type = parse_type(first, end);
                if (!m_type) return std::nullopt;

                result = ast::type(ast::fn_type {
                    .from = std::make_unique<ast::type>(std::move(result)),
                    .to = std::make_unique<ast::type>(std::move(*m_type)),
                });
            }

            iter = first;

            return result;
        }

        auto parse_expr(token const*& iter, token const* end) -> std::optional<ast::expr>;

        constexpr auto parse_lambda
            = [](token const*& iter, token const* end) -> std::optional<ast::lambda_expr> {
            if (iter == end) return std::nullopt;
            auto first = iter;
            // lambda_expr ::= "\" "(" list(identifier, ",") ")" "->" expr

            if (!parse_literal<token_type::lambda>(first, end)) return std::nullopt;
            if (!parse_literal<token_type::lcall>(first, end)) return std::nullopt;

            std::vector<ast::identifier> params;
            while (auto m_arg = parse_identifier(first, end)) {
                params.emplace_back(std::move(*m_arg));

                if (!parse_literal<token_type::list_sep>(first, end)) break;
            }

            if (!parse_literal<token_type::rcall>(first, end)) return std::nullopt;
            if (!parse_literal<token_type::farrow>(first, end)) return std::nullopt;

            auto m_expr = parse_expr(first, end);
            if (!m_expr) return std::nullopt;

            iter = first;
            return ast::lambda_expr {
                .params = std::move(params),
                .body = std::make_unique<ast::expr>(std::move(*m_expr)),
            };
        };

        auto parse_expr(token const*& iter, token const* end) -> std::optional<ast::expr> {
            if (iter == end) return std::nullopt;
            auto first = iter;
            // expr ::= int | real | string | identifier | fn_call_expr | lambda_expr | "(" expr ")"
            // fn_call_expr ::= expr "(" list(expr, ",") ")"
            // lambda_expr ::= "\" "(" list(identifier, ",") ")" "->" expr

            // expr ::= simple expr'
            // simple ::= int | real | string | identifier | "\" | "("
            // expr' ::= embellish trailing
            // embellish ::= Empty | (?<="\") "(" lambda_expr'
            // lambda_expr' ::= list(identifier, ",") ")" "->" expr
            // trailing ::= Empty | "(" fn_call_expr'
            // fn_call_expr' ::= list(expr, ",") ")"

            ast::expr result;

            if (auto m_int = parse_token<token_type::integer>(first, end)) {
                // TODO: parse string -> int64_t
            } else if (auto m_real = parse_token<token_type::real>(first, end)) {
                // TODO: parse string -> long double
            } else if (auto m_string = parse_token<token_type::string>(first, end)) {
                result.value = ast::string_lit_expr(std::string(m_string->text));
            } else if (auto m_id = parse_identifier(first, end)) {
                result.value = ast::identifier(std::move(*m_id));
            } else if (first->type == token_type::lambda) {
                auto m_lambda = parse_lambda(first, end);
                if (!m_lambda) return std::nullopt;
                result.value = std::move(*m_lambda);
            } else if (parse_literal<token_type::lgroup>(first, end)) {
                auto m_expr = parse_expr(first, end);
                if (!m_expr) return std::nullopt;
                result = std::move(*m_expr);

                if (!parse_literal<token_type::rgroup>(first, end)) return std::nullopt;
            } else {
                return std::nullopt;
            }

            while (parse_literal<token_type::lcall>(first, end)) {
                std::vector<ast::expr> args;

                while (auto m_expr = parse_expr(first, end)) {
                    args.emplace_back(std::move(*m_expr));
                    if (!parse_literal<token_type::list_sep>(first, end)) break;
                }

                result = ast::expr(ast::fn_call_expr {
                    .fn = std::make_unique<ast::expr>(std::move(result)),
                    .args = std::move(args),
                });

                if (!parse_literal<token_type::rcall>(first, end)) return std::nullopt;
            }

            iter = first;
            return result;
        }

        constexpr auto parse_fn_decl
            = [](token const*& iter, token const* end) -> std::optional<ast::fn_decl> {
            if (iter == end) return std::nullopt;
            auto first = iter;
            // fn_decl ::= "def" "::" type identifier "(" list(identifier, ",") ")" "=" expr ";"

            if (!parse_literal<token_type::def_fn>(first, end)) return std::nullopt;
            if (!parse_literal<token_type::typed_as>(first, end)) return std::nullopt;

            auto m_type = parse_type(first, end);
            if (!m_type) return std::nullopt;

            auto m_name = parse_identifier(first, end);
            if (!m_name) return std::nullopt;

            if (!parse_literal<token_type::lcall>(first, end)) return std::nullopt;

            std::vector<ast::identifier> params;
            while (auto m_arg = parse_identifier(first, end)) {
                params.emplace_back(std::move(*m_arg));

                if (!parse_literal<token_type::list_sep>(first, end)) break;
            }

            if (!parse_literal<token_type::rcall>(first, end)) return std::nullopt;
            if (!parse_literal<token_type::bind>(first, end)) return std::nullopt;

            auto m_expr = parse_expr(first, end);
            if (!parse_literal<token_type::stmt_term>(first, end)) return std::nullopt;

            iter = first;
            return ast::fn_decl {
                .name = std::move(*m_name),
                .type = std::move(*m_type),
                .params = std::move(params),
                .body = std::move(*m_expr),
            };
        };

        constexpr auto parse_struct_decl
            = [](token const*& iter, token const* end) -> std::optional<ast::struct_decl> {
            if (iter == end) return std::nullopt;
            // struct_decl ::= "struct" identifier "{" list(identifier "::" type, ",") "}"

            auto first = iter;

            if (!parse_literal<token_type::def_struct>(first, end)) return std::nullopt;
            if (first == end) return std::nullopt;

            assert(first->type == token_type::identifier);
            auto m_name = parse_identifier(first, end);
            if (first == end) return std::nullopt;
            if (!m_name) return std::nullopt;

            if (!parse_literal<token_type::lattr_list>(first, end)) return std::nullopt;
            if (first == end) return std::nullopt;

            std::vector<ast::attribute_decl> attributes;

            while (auto m_attr = parse_identifier(first, end)) {
                if (!parse_literal<token_type::typed_as>(first, end)) return std::nullopt;
                if (first == end) return std::nullopt;

                auto m_type = parse_type(first, end);
                if (first == end) return std::nullopt;
                if (!m_type) return std::nullopt;

                attributes.emplace_back(std::move(*m_attr), std::move(*m_type));

                if (!parse_literal<token_type::list_sep>(first, end)) { break; }
                if (first == end) return std::nullopt;
            }
            if (first == end) return std::nullopt;

            if (!parse_literal<token_type::rattr_list>(first, end)) return std::nullopt;
            if (first == end) return std::nullopt;

            iter = first;

            return ast::struct_decl {
                std::move(*m_name),
                std::move(attributes),
            };
        };

        constexpr auto parse_decl
            = [](token const*& iter, token const* end) -> std::optional<ast::declaration> {
            if (iter == end) return std::nullopt;
            // decl ::= struct_decl | fn_decl
            if (iter->type == token_type::def_struct) {
                auto x = parse_struct_decl(iter, end);
                if (x) { return ast::declaration(std::move(*x)); }
            } else if (iter->type == token_type::def_fn) {
                auto x = parse_fn_decl(iter, end);
                if (x) { return ast::declaration(std::move(*x)); }
            }

            return std::nullopt;
        };
    }

    ast::file parse(std::span<token const> tokens) {
        ast::file result;

        token const* first = tokens.data();
        token const* const last = tokens.data() + tokens.size();

        while (auto m_decl = parse_decl(first, last)) {
            result.value.emplace_back(std::move(*m_decl));
        }

        return result;
    }
}
