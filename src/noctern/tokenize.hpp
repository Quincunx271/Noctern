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
    // The token identifier.
    enum class token : uint8_t {
    // X-macro for the meaningful enumeration values.
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

        // A sentinel which is "empty", but doesn't need to be handled (can't ever result).
        empty_invalid,
    };

    // Converts a `token` to string.
    // Okay to call via ADL.
    constexpr inline std::string_view stringify(token token) {
        switch (token) {
#define NOCTERN_TOKEN_STR(name)                                                                    \
    case token::name: return #name;
            NOCTERN_X_TOKEN(NOCTERN_TOKEN_STR)
#undef NOCTERN_TOKEN_STR
        }
        assert(false);
    }

    // Allows string literals as template parameters.
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

        // Implementation detail; do not access.
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

    // Passes all the `token` enumeration values as arguments to `fn`.
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

    // Lifts a switch on `token` enumeration values into template parameters.
    //
    // In other words:
    //
    // ```
    // switch (token) {
    //   using enum noctern::token;
    // case enum_value_1: return fn(val<enum_value_1>);
    // // ...
    // }
    // ```
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

    ///////////////
    // "Data" definitions.
    //
    // This is the mechanism by which we associate data with a token.
    ///

    // Indicates that the associated data for the token is known at compile time.
    template <fixed_string S>
    struct empty_data {
        static constexpr std::string_view value = S;
    };

    template <typename T>
    inline constexpr bool is_empty_data = requires(T t) { []<auto V>(empty_data<V>) { }(t); };

    // Indicates that the associated data for the token is a runtime string.
    struct string_data { };

    // Specialized for each `token` enumeration.
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

    // Whether there is runtime data associated with this `token` type.
    constexpr bool has_data(token token) {
        return token_switch(token, []<noctern::token token>(val_t<token>) {
            return !is_empty_data<std::remove_cvref_t<decltype(token_data<token>)>>;
        });
    }

    class token_without_data {
    public:
        template <token token>
            requires(token == token::empty_invalid || !has_data(token))
        explicit constexpr token_without_data(val_t<token>)
            : value(token) {
        }

        token value;
    };

    class token_with_data {
    public:
        template <token token>
            requires(has_data(token))
        explicit constexpr token_with_data(val_t<token>)
            : value(token) {
        }

        token value;
    };

    // The result of calling `tokenize_all`.
    //
    // Holds references to the input string.
    class tokens {
    public:
        class builder {
            friend class tokens;

        public:
            void add_token(token_with_data token, std::string_view string_data) {
                tokens_.push_back(token.value);
                string_data_.push_back(string_data);
            }

            void add_token(token_without_data token) {
                tokens_.push_back(token.value);
            }

            template <token V, typename... Args>
            void add_token(val_t<V> token, Args... args) {
                if constexpr (std::constructible_from<token_with_data, val_t<V>>) {
                    add_token(token_with_data(token), args...);
                } else {
                    add_token(token_without_data(token), args...);
                }
            }

        private:
            std::vector<token> tokens_;
            std::vector<std::string_view> string_data_;
        };

        explicit tokens(builder builder)
            : tokens_(std::move(builder.tokens_))
            , string_data_(std::move(builder.string_data_)) {
        }

        // Runs a linear scan through the tokens, calling `fn` for each.
        //
        // `fn` is called like `fn(noctern::token{}, std::string_view{});`.
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
        friend class builder;

        // The parser only needs to work directly on the tokens. We organize memory to encourage
        // this.
        //
        // Not every token has runtime string data, so we don't store the data if it doesn't matter.
        // Note that we could actually optimize the data even more, at the expense of compute: we
        // don't need to store a full `string_view`, but only an offset into the original buffer. We
        // could retokenize to determine the string value.

        std::vector<token> tokens_;
        std::vector<std::string_view> string_data_;
    };

    tokens tokenize_all(std::string_view input);
}
