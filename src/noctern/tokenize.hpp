#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace noctern {
    enum class token : uint8_t {
#define NOCTERN_X_TOKEN(X)                                                                         \
    X(invalid)                                                                                     \
    X(space)                                                                                       \
                                                                                                   \
    X(comma)                                                                                       \
    X(lbrace)                                                                                      \
    X(rbrace)                                                                                      \
    X(lparen)                                                                                      \
    X(rparen)                                                                                      \
    X(statement_end)                                                                               \
                                                                                                   \
    X(fn_intro)                                                                                    \
    X(fn_outro)                                                                                    \
                                                                                                   \
    X(valdef_intro)                                                                                \
    X(valdef_outro)                                                                                \
    X(ident)                                                                                       \
                                                                                                   \
    X(int_lit)                                                                                     \
    X(real_lit)                                                                                    \
                                                                                                   \
    X(plus)                                                                                        \
    X(minus)                                                                                       \
    X(mult)                                                                                        \
    X(div)                                                                                         \
                                                                                                   \
    X(return_)
#define NOCTERN_MAKE_ENUM_VALUE(name) name,
        NOCTERN_X_TOKEN(NOCTERN_MAKE_ENUM_VALUE)
#undef NOCTERN_MAKE_ENUM_VALUE
    };

    constexpr inline std::string_view stringify(token token) {
        switch (token) {
#define NOCTERN_TOKEN_STR(name)                                                                    \
    case token::name: return #name;
            NOCTERN_X_TOKEN(NOCTERN_TOKEN_STR)
#undef NOCTERN_TOKEN_STR
        }
        assert(false);
    }

    template <size_t N>
    class fixed_string {
    public:
        constexpr fixed_string(const char (&str)[N + 1]) {
            for (size_t i = 0; i < N; ++i) {
                _chars[i] = str[i];
            }
        }

        constexpr operator std::string_view() const {
            return std::string_view(_chars, N);
        }

        char _chars[N + 1] = {}; // +1 for null terminator
    };

    template <size_t N>
    fixed_string(const char (&arr)[N]) -> fixed_string<N - 1>; // Drop the null terminator

    template <auto V>
    struct val_t {
        static constexpr auto value = V;
    };

    template <auto V>
    inline constexpr auto val = val_t<V> {};

    template <fixed_string S>
    inline constexpr auto val<S> = val_t<S> {};

    template <typename Fn>
    constexpr decltype(auto) all_tokens(Fn&& fn) {
        using enum token;
        return std::invoke(std::forward<Fn>(fn)
        // clang-format off
#define NOCTERN_TOKEN_TYPE(name) , val<name>
            // clang-format on
            NOCTERN_X_TOKEN(NOCTERN_TOKEN_TYPE)
#undef NOCTERN_TOKEN_TYPE
        );
    }

    template <typename Fn>
    constexpr decltype(auto) token_switch(token token, Fn&& fn) {
        switch (token) {
#define NOCTERN_TOKEN_CASE(name)                                                                   \
    case token::name: return std::invoke(std::forward<Fn>(fn), val<token::name>);
            NOCTERN_X_TOKEN(NOCTERN_TOKEN_CASE)
#undef NOCTERN_TOKEN_CASE
        }
        assert(false);
    }

#undef NOCTERN_X_TOKEN

    template <fixed_string S>
    struct empty_data {
        static constexpr std::string_view value = S;
    };

    template <typename T>
    inline constexpr bool is_empty_data = requires(T t) { []<auto V>(empty_data<V>) { }(t); };

    struct string_data { };

    template <token token>
    inline constexpr auto token_data = nullptr;

    template <>
    inline constexpr auto token_data<token::invalid> = string_data {};

    template <>
    inline constexpr auto token_data<token::space> = string_data {};

    template <>
    inline constexpr auto token_data<token::fn_intro> = empty_data<"def"> {};

    template <>
    inline constexpr auto token_data<token::fn_outro> = empty_data<":"> {};

    template <>
    inline constexpr auto token_data<token::lbrace> = empty_data<"{"> {};

    template <>
    inline constexpr auto token_data<token::rbrace> = empty_data<"}"> {};

    template <>
    inline constexpr auto token_data<token::comma> = empty_data<","> {};

    template <>
    inline constexpr auto token_data<token::valdef_intro> = empty_data<"let"> {};

    template <>
    inline constexpr auto token_data<token::valdef_outro> = empty_data<"="> {};

    template <>
    inline constexpr auto token_data<token::ident> = string_data {};

    template <>
    inline constexpr auto token_data<token::int_lit> = string_data {};

    template <>
    inline constexpr auto token_data<token::real_lit> = string_data {};

    template <>
    inline constexpr auto token_data<token::plus> = empty_data<"+"> {};

    template <>
    inline constexpr auto token_data<token::minus> = empty_data<"-"> {};

    template <>
    inline constexpr auto token_data<token::mult> = empty_data<"*"> {};

    template <>
    inline constexpr auto token_data<token::div> = empty_data<"/"> {};

    template <>
    inline constexpr auto token_data<token::lparen> = empty_data<"("> {};

    template <>
    inline constexpr auto token_data<token::rparen> = empty_data<")"> {};

    template <>
    inline constexpr auto token_data<token::statement_end> = empty_data<";"> {};

    template <>
    inline constexpr auto token_data<token::return_> = empty_data<"return"> {};

    constexpr bool has_data(token token) {
        return token_switch(token, []<noctern::token token>(val_t<token>) {
            return !is_empty_data<std::remove_cvref_t<decltype(token_data<token>)>>;
        });
    }

    class tokens {
    public:
        template <typename Fn>
        void walk(Fn&& fn) const {
            size_t string_data_index = 0;

            for (const token token : tokens_) {
                auto known_str = token_switch(token,
                    []<noctern::token token>(val_t<token>) -> std::optional<std::string_view> {
                        using token_data_type = std::remove_cvref_t<decltype(token_data<token>)>;

                        if constexpr (is_empty_data<token_data_type>) {
                            return token_data_type::value;
                        } else {
                            return std::nullopt;
                        }
                    });
                if (!known_str.has_value()) {
                    known_str = string_data_[string_data_index];
                    ++string_data_index;
                }

                std::invoke(fn, token, std::string_view(*known_str));
            }
        }

    private:
        friend tokens tokenize_all(std::string_view input);

        std::vector<token> tokens_;
        std::vector<std::string_view> string_data_;
    };

    std::pair<token, std::string_view> tokenize_one(std::string_view input);

    tokens tokenize_all(std::string_view input);
}
