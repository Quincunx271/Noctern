#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "noctern/enum.hpp"
#include "noctern/iterator_facade.hpp"
#include "noctern/meta.hpp"

namespace noctern {
    struct _token_id_wrapper {
        // The token_id identifier.
        enum class token_id : uint8_t {
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

    private:
        friend enum_mixin;

        template <typename Fn>
        friend constexpr decltype(auto) switch_introspect(token_id t, Fn&& fn) {
            switch (t) {
                using enum token_id;
                NOCTERN_X_TOKEN(NOCTERN_ENUM_X_INTROSPECT)
            case token_id::empty_invalid: assert(false);
            }
            assert(false);
        }

        template <typename Fn>
        friend constexpr decltype(auto) introspect(type_t<token_id>, Fn&& fn) {
            using enum token_id;
            return std::invoke(std::forward<Fn>(fn)
#define NOCTERN_TOKEN_TYPE(name) , val<name>
                    NOCTERN_X_TOKEN(NOCTERN_TOKEN_TYPE)
#undef NOCTERN_TOKEN_TYPE
            );
        }
#undef NOCTERN_X_TOKEN
    };

    using token_id = _token_id_wrapper::token_id;

    ///////////////
    // "Data" definitions.
    //
    // This is the mechanism by which we associate data with a token_id.
    ///

    // Indicates that the associated data for the token_id is known at compile time.
    template <fixed_string S>
    struct empty_data {
        static constexpr std::string_view value = S;
    };

    template <typename T>
    inline constexpr bool is_empty_data = requires(T t) { []<auto V>(empty_data<V>) { }(t); };

    // Indicates that the associated data for the token_id is a runtime string.
    struct string_data { };

    // Specialized for each `token_id` enumeration.
    template <token_id token_id>
    inline constexpr auto token_data = nullptr;

    template <>
    inline constexpr auto token_data<token_id::invalid> = string_data {};

    template <>
    inline constexpr auto token_data<token_id::space> = string_data {};

    template <>
    inline constexpr auto token_data<token_id::fn_intro> = empty_data<"def"> {};

    template <>
    inline constexpr auto token_data<token_id::fn_outro> = empty_data<":"> {};

    template <>
    inline constexpr auto token_data<token_id::lbrace> = empty_data<"{"> {};

    template <>
    inline constexpr auto token_data<token_id::rbrace> = empty_data<"}"> {};

    template <>
    inline constexpr auto token_data<token_id::comma> = empty_data<","> {};

    template <>
    inline constexpr auto token_data<token_id::valdef_intro> = empty_data<"let"> {};

    template <>
    inline constexpr auto token_data<token_id::valdef_outro> = empty_data<"="> {};

    template <>
    inline constexpr auto token_data<token_id::ident> = string_data {};

    template <>
    inline constexpr auto token_data<token_id::int_lit> = string_data {};

    template <>
    inline constexpr auto token_data<token_id::real_lit> = string_data {};

    template <>
    inline constexpr auto token_data<token_id::plus> = empty_data<"+"> {};

    template <>
    inline constexpr auto token_data<token_id::minus> = empty_data<"-"> {};

    template <>
    inline constexpr auto token_data<token_id::mult> = empty_data<"*"> {};

    template <>
    inline constexpr auto token_data<token_id::div> = empty_data<"/"> {};

    template <>
    inline constexpr auto token_data<token_id::lparen> = empty_data<"("> {};

    template <>
    inline constexpr auto token_data<token_id::rparen> = empty_data<")"> {};

    template <>
    inline constexpr auto token_data<token_id::statement_end> = empty_data<";"> {};

    template <>
    inline constexpr auto token_data<token_id::return_> = empty_data<"return"> {};

    // Whether there is runtime data associated with this `token_id` type.
    constexpr bool has_data(token_id token_id) {
        return enum_switch(token_id, []<noctern::token_id token_id>(val_t<token_id>) {
            return !is_empty_data<std::remove_cvref_t<decltype(token_data<token_id>)>>;
        });
    }

    class token_without_data {
    public:
        template <token_id token_id>
            requires(token_id == token_id::empty_invalid || !has_data(token_id))
        explicit constexpr token_without_data(val_t<token_id>)
            : value(token_id) {
        }

        token_id value;
    };

    class token_with_data {
    public:
        template <token_id token_id>
            requires(has_data(token_id))
        explicit constexpr token_with_data(val_t<token_id>)
            : value(token_id) {
        }

        token_id value;
    };

    // The result of calling `tokenize_all`.
    //
    // Holds references to the input string.
    class tokens {
    public:
        friend class const_iterator;
        class const_iterator : public iterator_facade<const_iterator> {
            friend tokens;

        public:
            struct token_and_str {
                noctern::token_id token_id;
                std::string_view string_data;
            };

            constexpr const_iterator() = default;

            token_and_str read() const {
                noctern::token_id token_id = tokens_->tokens_[index_];

                auto str_data = enum_switch(token_id,
                    []<noctern::token_id token_id>(
                        val_t<token_id>) -> std::optional<std::string_view> {
                        using token_data_type = std::remove_cvref_t<decltype(token_data<token_id>)>;

                        if constexpr (is_empty_data<token_data_type>) {
                            return token_data_type::value;
                        } else {
                            return std::nullopt;
                        }
                    });
                if (!str_data.has_value()) {
                    str_data = tokens_->string_data_[string_data_index_];
                }
                return {token_id, *str_data};
            }

            template <ptrdiff_t N>
                requires(N == 1 || N == -1)
            void advance(val_t<N>) {
                if constexpr (N == 1) {
                    if (has_data(tokens_->tokens_[index_])) {
                        ++string_data_index_;
                    }
                }
                index_ += N;
                if constexpr (N == -1) {
                    if (has_data(tokens_->tokens_[index_])) {
                        --string_data_index_;
                    }
                }
            }

            ptrdiff_t distance(const_iterator rhs) const {
                return rhs.index_ - index_;
            }

        private:
            explicit constexpr const_iterator(
                const tokens* tokens, ptrdiff_t index, size_t string_data_index)
                : tokens_(tokens)
                , index_(index)
                , string_data_index_(string_data_index) {
            }

            const tokens* tokens_ = nullptr;
            ptrdiff_t index_ = 0;
            size_t string_data_index_ = 0;
        };

        class builder {
            friend class tokens;

        public:
            void add_token(token_with_data token_id, std::string_view string_data) {
                tokens_.push_back(token_id.value);
                string_data_.push_back(string_data);
            }

            void add_token(token_without_data token_id) {
                tokens_.push_back(token_id.value);
            }

            template <token_id V, typename... Args>
            void add_token(val_t<V> token_id, Args... args) {
                if constexpr (std::constructible_from<token_with_data, val_t<V>>) {
                    add_token(token_with_data(token_id), args...);
                } else {
                    add_token(token_without_data(token_id), args...);
                }
            }

        private:
            std::vector<token_id> tokens_;
            std::vector<std::string_view> string_data_;
        };

        explicit tokens(builder builder)
            : tokens_(std::move(builder.tokens_))
            , string_data_(std::move(builder.string_data_)) {
        }

        size_t num_tokens() const {
            return tokens_.size();
        }

        const_iterator begin() const {
            return const_iterator(this, 0, 0);
        }

        const_iterator end() const {
            return const_iterator(this, tokens_.size(), string_data_.size());
        }

        // Runs a linear scan through the tokens, calling `fn` for each.
        //
        // `fn` is called like `fn(noctern::token_id{}, std::string_view{});`.
        //
        // The `string_view` values live as long as the `tokens` object.
        template <typename Fn>
        void walk(Fn&& fn) const {
            for (const const_iterator::token_and_str token_id : *this) {
                std::invoke(fn, token_id.token_id, token_id.string_data);
            }
        }

    private:
        friend class builder;

        // The parser only needs to work directly on the tokens. We organize memory to encourage
        // this.
        //
        // Not every token_id has runtime string data, so we don't store the data if it doesn't
        // matter. Note that we could actually optimize the data even more, at the expense of
        // compute: we don't need to store a full `string_view`, but only an offset into the
        // original buffer. We could retokenize to determine the string value.

        std::vector<token_id> tokens_;
        std::vector<std::string_view> string_data_;
    };
    static_assert(std::bidirectional_iterator<tokens::const_iterator>);

    tokens tokenize_all(std::string_view input);
}
