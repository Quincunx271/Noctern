#include "./tokenize.hpp"

#include <string>

#include <catch2/catch.hpp>

#include "./tokenize.test.hpp"

namespace noctern {
    namespace {
        using namespace std::literals;

        struct test_case {
            std::string name;

            std::string input = "";
            std::vector<token> expected = {};
            std::vector<std::string> expected_str_data = {};
        };

        TEST_CASE("tokenize_all works") {
            std::vector<test_case> test_cases = {
                {.name = "empty"},
                {
                    .name = "fn_intro",
                    .input = "def",
                    .expected = {token::fn_intro},
                    .expected_str_data = {"def"},
                },
                {
                    .name = "fn_outro",
                    .input = ":",
                    .expected = {token::fn_outro},
                    .expected_str_data = {":"},
                },
                {
                    .name = "lbrace",
                    .input = "{",
                    .expected = {token::lbrace},
                    .expected_str_data = {"{"},
                },
                {
                    .name = "rbrace",
                    .input = "}",
                    .expected = {token::rbrace},
                    .expected_str_data = {"}"},
                },
                {
                    .name = "valdef_intro",
                    .input = "let",
                    .expected = {token::valdef_intro},
                    .expected_str_data = {"let"},
                },
                {
                    .name = "valdef_outro",
                    .input = "=",
                    .expected = {token::valdef_outro},
                    .expected_str_data = {"="},
                },
                {
                    .name = "ident/ordinary",
                    .input = "foo",
                    .expected = {token::ident},
                    .expected_str_data = {"foo"},
                },
                {
                    .name = "ident/camel",
                    .input = "myVar",
                    .expected = {token::ident},
                    .expected_str_data = {"myVar"},
                },
                {
                    .name = "ident/Pascal",
                    .input = "MyVar",
                    .expected = {token::ident},
                    .expected_str_data = {"MyVar"},
                },
                {
                    .name = "ident/snake",
                    .input = "my_var_special0",
                    .expected = {token::ident},
                    .expected_str_data = {"my_var_special0"},
                },
                {
                    .name = "ident/starts with underscore",
                    .input = "_hi",
                    .expected = {token::ident},
                    .expected_str_data = {"_hi"},
                },
                {
                    .name = "ident/one character",
                    .input = "X",
                    .expected = {token::ident},
                    .expected_str_data = {"X"},
                },
                {
                    // Consider disallowing this.
                    .name = "non-ident/starts with number",
                    .input = "1var",
                    .expected = {token::int_lit, token::ident},
                    .expected_str_data = {"1", "var"},
                },
                {
                    .name = "int_lit/ordinary",
                    .input = "12345",
                    .expected = {token::int_lit},
                    .expected_str_data = {"12345"},
                },
                {
                    .name = "int_lit/leading zeros",
                    .input = "0001",
                    .expected = {token::int_lit},
                    .expected_str_data = {"0001"},
                },
                {
                    .name = "int_lit/zero",
                    .input = "0",
                    .expected = {token::int_lit},
                    .expected_str_data = {"0"},
                },
                {
                    .name = "real_lit/ordinary",
                    .input = "12345.67890",
                    .expected = {token::real_lit},
                    .expected_str_data = {"12345.67890"},
                },
                {
                    .name = "real_lit/no trailing digits",
                    .input = "12345.",
                    .expected = {token::real_lit},
                    .expected_str_data = {"12345."},
                },
                {
                    .name = "real_lit/no leading digits",
                    .input = ".1234",
                    .expected = {token::real_lit},
                    .expected_str_data = {".1234"},
                },
                {
                    .name = "real_lit/no digits",
                    .input = ".",
                    .expected = {token::real_lit},
                    .expected_str_data = {"."},
                },
                {
                    .name = "plus",
                    .input = "+",
                    .expected = {token::plus},
                    .expected_str_data = {"+"},
                },
                {
                    .name = "minus",
                    .input = "-",
                    .expected = {token::minus},
                    .expected_str_data = {"-"},
                },
                {
                    .name = "mult",
                    .input = "*",
                    .expected = {token::mult},
                    .expected_str_data = {"*"},
                },
                {
                    .name = "div",
                    .input = "/",
                    .expected = {token::div},
                    .expected_str_data = {"/"},
                },
                {
                    .name = "lparen",
                    .input = "(",
                    .expected = {token::lparen},
                    .expected_str_data = {"("},
                },
                {
                    .name = "rparen",
                    .input = ")",
                    .expected = {token::rparen},
                    .expected_str_data = {")"},
                },
                {
                    .name = "statement_end",
                    .input = ";",
                    .expected = {token::statement_end},
                    .expected_str_data = {";"},
                },
                {
                    .name = "return",
                    .input = "return",
                    .expected = {token::return_},
                    .expected_str_data = {"return"},
                },
            };

            for (char c : "azAZ"sv) {
                std::string s;
                s.push_back(c);
                test_cases.push_back({
                    .name = "ident/range boundaries/" + s,
                    .input = s,
                    .expected = {token::ident},
                    .expected_str_data = {s},
                });
            }
            for (char c : "azAZ09"sv) {
                std::string s = "x";
                s.push_back(c);
                test_cases.push_back({
                    .name = "ident/range boundaries/" + s,
                    .input = s,
                    .expected = {token::ident},
                    .expected_str_data = {s},
                });
            }
            for (char c : "0123456789"sv) {
                std::string s;
                s.push_back(c);
                test_cases.push_back({
                    .name = "int_lit/all individual digits/" + s,
                    .input = s,
                    .expected = {token::int_lit},
                    .expected_str_data = {s},
                });
            }

            std::ranges::sort(test_cases, std::ranges::less {},
                [](const test_case test_case) { return test_case.name; });

            test_cases.clear();
            test_cases.push_back({
                .name = "function definition stream",
                .input = R"(def foobar(x, y): { let z = y; return z   + x + 0.2; })",
                .expected = {token::fn_intro, token::space, token::ident, token::lparen,
                    token::ident, token::comma, token::space, token::ident, token::rparen,
                    token::fn_outro, token::space, token::lbrace, token::space, token::valdef_intro,
                    token::space, token::ident, token::space, token::valdef_outro, token::space,
                    token::ident, token::statement_end, token::space, token::return_, token::space,
                    token::ident, token::space, token::plus, token::space, token::ident,
                    token::space, token::plus, token::space, token::real_lit, token::statement_end,
                    token::space, token::rbrace},
                .expected_str_data = {"def", " ", "foobar", "(", "x", ",", " ", "y", ")", ":", " ",
                    "{", " ", "let", " ", "z", " ", "=", " ", "y", ";", " ", "return", " ", "z",
                    "   ", "+", " ", "x", " ", "+", " ", "0.2", ";", " ", "}"},
            });

            for (const test_case& test_case : test_cases) {
                SECTION(test_case.name) {
                    tokens tokens = tokenize_all(test_case.input);

                    std::vector<token> actual;
                    std::vector<std::string> actual_str_data;
                    tokens.walk([&](token token, std::string_view str_data) {
                        actual.push_back(token);
                        actual_str_data.emplace_back(str_data);
                    });

                    CHECK(actual == test_case.expected);
                    CHECK(actual_str_data == test_case.expected_str_data);
                }
            }
        }
    }
}