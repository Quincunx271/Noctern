#include <noctern/ast.hpp>

#include <noctern/debug_print.hpp>
#include <noctern/debug_print.test.hpp>

#include <cstddef>
#include <iterator>
#include <sstream>

#include <catch2/catch.hpp>

using namespace std::literals;

namespace ast = noctern::ast;
using noctern::debug_print;

using Catch::Matchers::Contains;

namespace {
    template <typename T, std::size_t N>
    std::vector<T> make_vec(T(&&init)[N]) {
        std::vector<T> result;
        result.reserve(N);

        using std::begin;
        using std::end;

        for (auto&& x : init) {
            result.emplace_back(std::move(x));
        }

        return result;
    }
}

TEST_CASE("Can print identifiers") {
    std::ostringstream out;
    auto value = ast::identifier("Hello"s);

    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("Hello"));
}

TEST_CASE("Can print basic_types") {
    std::ostringstream out;
    auto value = ast::basic_type(ast::identifier("TheType"s));

    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("TheType"));
}

TEST_CASE("Can print fn_types") {
    std::ostringstream out;
    auto value = ast::fn_type {
        std::make_unique<ast::type>(ast::basic_type(ast::identifier("TypeA"s))),
        std::make_unique<ast::type>(ast::basic_type(ast::identifier("TypeB"s))),
    };

    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("TypeA"));
    CHECK_THAT(out.str(), Contains("TypeB"));
}

TEST_CASE("Can print eval_types") {
    std::ostringstream out;
    auto value = ast::eval_type {
        ast::basic_type(ast::identifier("TheBaseType"s)),
        make_vec({
            ast::type(ast::basic_type(ast::identifier("TypeB"s))),
            ast::type(ast::basic_type(ast::identifier("TypeC"s))),
        }),
    };

    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("TheBaseType"));
    CHECK_THAT(out.str(), Contains("TypeB"));
    CHECK_THAT(out.str(), Contains("TypeC"));
}

TEST_CASE("Can print types") {
    std::ostringstream out;
    auto value = ast::type(ast::basic_type(ast::identifier("BasicType"s)));
    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("BasicType"));
}

TEST_CASE("Can print int_lit_expr") {
    std::ostringstream out;
    auto value = ast::int_lit_expr(-32);
    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("-32"));
}

TEST_CASE("Can print real_lit_expr") {
    std::ostringstream out;
    auto value = ast::real_lit_expr(128.5);
    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("128.5"));
}

TEST_CASE("Can print string_lit_expr") {
    std::ostringstream out;
    auto value = ast::string_lit_expr("Some string");
    out << debug_print(value);
    CHECK_THAT(out.str(), Contains("Some string"));
}
