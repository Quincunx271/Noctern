#include "./tokenize.hpp"

#include <algorithm>
#include <ranges>
#include <string>

#include <catch2/catch.hpp>

#include "./tokenize.test.hpp"

namespace noctern {
    namespace {
        using namespace std::literals;

        template <typename Range>
            requires std::ranges::input_range<Range>
        std::vector<std::ranges::range_value_t<Range>> to_vector(Range&& range) {
            return std::vector<std::ranges::range_value_t<Range>>(
                std::ranges::begin(range), std::ranges::end(range));
        }

        struct test_case {
            std::string name;

            std::string input = "";
            std::vector<token_id> expected = {};
            std::vector<std::string> expected_str_data = {};
        };

        TEST_CASE("tokenize_all works") {
            std::vector<test_case> test_cases = {
                {.name = "empty"},
                {
                    .name = "fn_intro",
                    .input = "def",
                    .expected = {token_id::fn_intro},
                    .expected_str_data = {"def"},
                },
                {
                    .name = "fn_outro",
                    .input = ":",
                    .expected = {token_id::fn_outro},
                    .expected_str_data = {":"},
                },
                {
                    .name = "lbrace",
                    .input = "{",
                    .expected = {token_id::lbrace},
                    .expected_str_data = {"{"},
                },
                {
                    .name = "rbrace",
                    .input = "}",
                    .expected = {token_id::rbrace},
                    .expected_str_data = {"}"},
                },
                {
                    .name = "valdef_intro",
                    .input = "let",
                    .expected = {token_id::valdef_intro},
                    .expected_str_data = {"let"},
                },
                {
                    .name = "valdef_outro",
                    .input = "=",
                    .expected = {token_id::valdef_outro},
                    .expected_str_data = {"="},
                },
                {
                    .name = "ident/ordinary",
                    .input = "foo",
                    .expected = {token_id::ident},
                    .expected_str_data = {"foo"},
                },
                {
                    .name = "ident/camel",
                    .input = "myVar",
                    .expected = {token_id::ident},
                    .expected_str_data = {"myVar"},
                },
                {
                    .name = "ident/Pascal",
                    .input = "MyVar",
                    .expected = {token_id::ident},
                    .expected_str_data = {"MyVar"},
                },
                {
                    .name = "ident/snake",
                    .input = "my_var_special0",
                    .expected = {token_id::ident},
                    .expected_str_data = {"my_var_special0"},
                },
                {
                    .name = "ident/starts with underscore",
                    .input = "_hi",
                    .expected = {token_id::ident},
                    .expected_str_data = {"_hi"},
                },
                {
                    .name = "ident/one character",
                    .input = "X",
                    .expected = {token_id::ident},
                    .expected_str_data = {"X"},
                },
                {
                    // Consider disallowing this.
                    .name = "non-ident/starts with number",
                    .input = "1var",
                    .expected = {token_id::int_lit, token_id::ident},
                    .expected_str_data = {"1", "var"},
                },
                {
                    .name = "int_lit/ordinary",
                    .input = "12345",
                    .expected = {token_id::int_lit},
                    .expected_str_data = {"12345"},
                },
                {
                    .name = "int_lit/leading zeros",
                    .input = "0001",
                    .expected = {token_id::int_lit},
                    .expected_str_data = {"0001"},
                },
                {
                    .name = "int_lit/zero",
                    .input = "0",
                    .expected = {token_id::int_lit},
                    .expected_str_data = {"0"},
                },
                {
                    .name = "real_lit/ordinary",
                    .input = "12345.67890",
                    .expected = {token_id::real_lit},
                    .expected_str_data = {"12345.67890"},
                },
                {
                    .name = "real_lit/no trailing digits",
                    .input = "12345.",
                    .expected = {token_id::real_lit},
                    .expected_str_data = {"12345."},
                },
                {
                    .name = "real_lit/no leading digits",
                    .input = ".1234",
                    .expected = {token_id::real_lit},
                    .expected_str_data = {".1234"},
                },
                {
                    .name = "real_lit/no digits",
                    .input = ".",
                    .expected = {token_id::real_lit},
                    .expected_str_data = {"."},
                },
                {
                    .name = "plus",
                    .input = "+",
                    .expected = {token_id::plus},
                    .expected_str_data = {"+"},
                },
                {
                    .name = "minus",
                    .input = "-",
                    .expected = {token_id::minus},
                    .expected_str_data = {"-"},
                },
                {
                    .name = "mult",
                    .input = "*",
                    .expected = {token_id::mult},
                    .expected_str_data = {"*"},
                },
                {
                    .name = "div",
                    .input = "/",
                    .expected = {token_id::div},
                    .expected_str_data = {"/"},
                },
                {
                    .name = "lparen",
                    .input = "(",
                    .expected = {token_id::lparen},
                    .expected_str_data = {"("},
                },
                {
                    .name = "rparen",
                    .input = ")",
                    .expected = {token_id::rparen},
                    .expected_str_data = {")"},
                },
                {
                    .name = "statement_end",
                    .input = ";",
                    .expected = {token_id::statement_end},
                    .expected_str_data = {";"},
                },
                {
                    .name = "return",
                    .input = "return",
                    .expected = {token_id::return_},
                    .expected_str_data = {"return"},
                },
            };

            for (char c : "azAZ"sv) {
                std::string s;
                s.push_back(c);
                test_cases.push_back({
                    .name = "ident/range boundaries/" + s,
                    .input = s,
                    .expected = {token_id::ident},
                    .expected_str_data = {s},
                });
            }
            for (char c : "azAZ09"sv) {
                std::string s = "x";
                s.push_back(c);
                test_cases.push_back({
                    .name = "ident/range boundaries/" + s,
                    .input = s,
                    .expected = {token_id::ident},
                    .expected_str_data = {s},
                });
            }
            for (char c : "0123456789"sv) {
                std::string s;
                s.push_back(c);
                test_cases.push_back({
                    .name = "int_lit/all individual digits/" + s,
                    .input = s,
                    .expected = {token_id::int_lit},
                    .expected_str_data = {s},
                });
            }

            std::ranges::sort(test_cases, std::ranges::less {},
                [](const test_case test_case) { return test_case.name; });

            test_cases.clear();
            test_cases.push_back({
                .name = "function definition stream",
                .input = R"(def foobar(x, y): { let z = y; return z   + x + 0.2; })",
                .expected = {token_id::fn_intro, token_id::space, token_id::ident, token_id::lparen,
                    token_id::ident, token_id::comma, token_id::space, token_id::ident,
                    token_id::rparen, token_id::fn_outro, token_id::space, token_id::lbrace,
                    token_id::space, token_id::valdef_intro, token_id::space, token_id::ident,
                    token_id::space, token_id::valdef_outro, token_id::space, token_id::ident,
                    token_id::statement_end, token_id::space, token_id::return_, token_id::space,
                    token_id::ident, token_id::space, token_id::plus, token_id::space,
                    token_id::ident, token_id::space, token_id::plus, token_id::space,
                    token_id::real_lit, token_id::statement_end, token_id::space, token_id::rbrace},
                .expected_str_data = {"def", " ", "foobar", "(", "x", ",", " ", "y", ")", ":", " ",
                    "{", " ", "let", " ", "z", " ", "=", " ", "y", ";", " ", "return", " ", "z",
                    "   ", "+", " ", "x", " ", "+", " ", "0.2", ";", " ", "}"},
            });

            for (const test_case& test_case : test_cases) {
                SECTION(test_case.name) {
                    tokens tokens = tokenize_all(test_case.input);

                    std::vector<token_id> actual;
                    std::vector<std::string> actual_str_data;
                    for (const token token : tokens) {
                        actual.push_back(tokens.id(token));
                        actual_str_data.emplace_back(tokens.string(token));
                    }

                    std::vector<token_id> expected_ids(to_vector(std::ranges::filter_view(
                        test_case.expected, [](token_id id) { return id != token_id::space; })));
                    std::vector<std::string> expected_str_data(to_vector(
                        std::ranges::zip_view(test_case.expected, test_case.expected_str_data)
                        | std::ranges::views::filter(
                            [](const auto& token) { return std::get<0>(token) != token_id::space; })
                        | std::ranges::views::transform(
                            [](const auto& token) { return std::get<1>(token); })));

                    CHECK(actual == expected_ids);
                    CHECK(actual_str_data == expected_str_data);
                }
                SECTION(test_case.name + " with spaces") {
                    tokens tokens = tokenize_all_keeping_spaces(test_case.input);

                    std::vector<token_id> actual;
                    std::vector<std::string> actual_str_data;
                    for (const token token : tokens) {
                        actual.push_back(tokens.id(token));
                        actual_str_data.emplace_back(tokens.string(token));
                    }

                    CHECK(actual == test_case.expected);
                    CHECK(actual_str_data == test_case.expected_str_data);
                }
            }
        }
    }
}