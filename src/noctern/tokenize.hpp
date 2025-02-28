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
    using token_index_t = int32_t;

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

    class token {
        friend class tokens;

    public:
        constexpr token() = default;

    private:
        explicit constexpr token(token_index_t index)
            : index_(index) {
        }

        token_index_t index_ = static_cast<token_index_t>(-1);
    };

    // The result of calling `tokenize_all`.
    //
    // Holds references to the input string.
    class tokens {
        static constexpr token make(token_index_t index) {
            return token {index};
        }

    public:
        friend class const_iterator;
        class const_iterator : public iterator_facade<const_iterator> {
            friend tokens;

        public:
            using difference_type = token_index_t;

            constexpr const_iterator() = default;

            token read() const {
                return make(index_);
            }

            void advance(token_index_t offset) {
                index_ += offset;
            }

            ptrdiff_t distance(const_iterator rhs) const {
                return rhs.index_ - index_;
            }

        private:
            explicit constexpr const_iterator(const tokens* tokens, token_index_t index)
                : tokens_(tokens)
                , index_(index) {
            }

            const tokens* tokens_ = nullptr;
            token_index_t index_ = 0;
        };

        class builder {
            friend class tokens;

        public:
            explicit constexpr builder(std::string_view input_file)
                : remaining_input_(input_file)
                , input_file_(input_file) {
            }

            std::string_view remaining_input() const {
                return remaining_input_;
            }

            void add_token(token_id token, token_index_t length) {
                tokens_.push_back(token);
                token_strs_.emplace_back(remaining_input_.substr(0, length));
                input_start_index_ += length;
                remaining_input_.remove_prefix(length);
            }

            void add_ignored_token(token_index_t length) {
                input_start_index_ += length;
                remaining_input_.remove_prefix(length);
            }

        private:
            std::string_view remaining_input_;

            std::string_view input_file_;
            token_index_t input_start_index_ = 0;

            std::vector<token_id> tokens_;
            std::vector<std::string_view> token_strs_;
        };

        explicit tokens(builder builder)
            : input_file_(builder.input_file_)
            , tokens_(std::move(builder.tokens_))
            , token_strs_(std::move(builder.token_strs_)) {
        }

        size_t num_tokens() const {
            return tokens_.size();
        }

        const_iterator begin() const {
            return const_iterator(this, 0);
        }

        const_iterator end() const {
            return const_iterator(this, tokens_.size());
        }

        class extracted_data {
            friend tokens;

        public:
            token_id id;

        private:
            std::string_view string;
        };

        extracted_data extract(const_iterator pos) {
            extracted_data result;
            result.id = std::exchange(tokens_[pos.index_], token_id::invalid);
            result.string = token_strs_[pos.index_];
            return result;
        }

        void store(const_iterator dest, extracted_data source) {
            tokens_[dest.index_] = source.id;
            token_strs_[dest.index_] = source.string;
        }

        void erase_to_end(const_iterator pos) {
            tokens_.erase(tokens_.begin() + pos.index_, tokens_.end());
            token_strs_.erase(token_strs_.begin() + pos.index_, token_strs_.end());
        }

        const_iterator to_iterator(token token) const {
            return const_iterator(this, token.index_);
        }

        token_id id(token token) const {
            return tokens_[token.index_];
        }

        std::string_view string(token token) const {
            return token_strs_[token.index_];
        }

    private:
        friend class builder;

        std::string_view input_file_;

        // The parser only needs to work directly on the tokens. We organize memory to encourage
        // this.
        //
        // Not every token_id has runtime string data, so we don't store the data if it doesn't
        // matter. Note that we could actually optimize the data even more, at the expense of
        // compute: we don't need to store a full `string_view`, but only an offset into the
        // original buffer. We could retokenize to determine the string value.

        std::vector<token_id> tokens_;

        // This representation is probably wrong: tokens are an incrementing index, rather than a
        // direct index into the source file. We can change this, but this is a slightly easier
        // initial representation.

        std::vector<std::string_view> token_strs_;
    };
    static_assert(std::bidirectional_iterator<tokens::const_iterator>);

    tokens tokenize_all(std::string_view input);

    tokens tokenize_all_keeping_spaces(std::string_view input);
}
