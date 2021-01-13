#include <concepts>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <noctern/parsing/lexer.hpp>
#include <noctern/parsing/parser.hpp>

namespace ast = noctern::ast;

template <typename T, typename... Ts>
concept AnyOf = (std::same_as<T, Ts> || ...);

void pp(std::ostream& out, ast::basic_type const& type) {
    out << type.value.value;
}

void pp(std::ostream& out, ast::type const& type);
void pp(std::ostream& out, ast::fn_type const& type) {
    pp(out, *type.from);
    out << " -> ";
    pp(out, *type.to);
}

void pp(std::ostream& out, ast::eval_type const& type) {
    pp(out, type.base);
    out << "[";

    bool first = true;
    for (auto const& arg : type.args) {
        if (!first) out << ", ";
        first = false;
        pp(out, arg);
    }

    out << "]";
}

void pp(std::ostream& out, ast::type const& type) {
    std::visit([&](auto const& x) { pp(out, x); }, type.value);
}

void pp(std::ostream& out, ast::struct_decl const& decl, int indent) {
    out << "struct " << decl.name.value << " {\n";
    for (auto const& attr : decl.attributes) {
        out << "  " << attr.name.value << " :: ";
        pp(out, attr.type);
    }
    out << "}\n";
}

void pp(std::ostream& out,
    AnyOf<ast::int_lit_expr, ast::real_lit_expr, ast::string_lit_expr, ast::identifier> auto const&
        expr,
    int indent) {
    out << expr.value;
}

void pp(std::ostream& out, ast::expr const& expr, int indent);

void pp(std::ostream& out, ast::fn_call_expr const& expr, int indent) {
    out << '(';
    pp(out, *expr.fn, indent);
    out << ')';
    out << '(';
    bool first = true;
    for (auto const& arg : expr.args) {
        if (!first) out << ", ";
        first = false;
        pp(out, arg, indent + 1);
    }
    out << ')';
}

void pp(std::ostream& out, ast::lambda_expr const& expr, int indent) {
    out << "\\(";

    bool first = true;
    for (auto const& param : expr.params) {
        if (!first) out << ", ";
        first = false;
        out << param.value;
    }

    out << ") -> ";
    pp(out, *expr.body, indent + 1);
}

void pp(std::ostream& out, ast::expr const& expr, int indent) {
    std::visit([&](auto const& x) { pp(out, x, indent); }, expr.value);
}

void pp(std::ostream& out, ast::fn_decl const& decl, int indent) {
    out << "def :: ";
    pp(out, decl.type);
    out << '\n';
    out << "    " << decl.name.value << "(";

    bool first = true;
    for (auto const& param : decl.params) {
        if (!first) out << ", ";
        first = false;
        out << param.value;
    }

    out << ") =\n";
    out << "      ";
    pp(out, decl.body, indent + 3);
    out << ";\n";
}

void pp(std::ostream& out, ast::declaration const& decl, int indent) {
    std::visit([&](auto const& x) { pp(out, x, indent); }, decl.value);
}

int main() {
    auto const input = [] {
        std::ostringstream out;
        out << std::cin.rdbuf();
        return out.str();
    }();

    noctern::lexer lex {input};
    std::vector<noctern::token> tokens(lex.begin(), lex.end());
    std::erase_if(tokens, [](noctern::token const& tok) {
        return tok.type == noctern::token_type::space || tok.type == noctern::token_type::newline
            || tok.type == noctern::token_type::line_comment;
    });

    ast::file result = noctern::parse(tokens);

    for (auto const& decl : result.value) {
        ::pp(std::cout, decl, 0);
        std::cout << '\n';
    }
}
