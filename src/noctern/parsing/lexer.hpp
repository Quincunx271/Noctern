#pragma once

#include <optional>
#include <string_view>

#include <noctern/data/token.hpp>
#include <noctern/utils/debug_print.hpp>

namespace noctern {
    class lexer {
    public:
        class iterator;
        friend class lexer::iterator;
        class sentinel { };

        explicit lexer(std::string_view text);
        lexer(lexer const&) = delete;
        ~lexer();

        iterator begin();

        iterator end() const;
        sentinel end_sentinel() const;

    private:
        // lexer-generator functions
        void advance();
        bool has_next() const;
        token const& read() const;
        // end of lexer-generator functions

        void* lexer_ = nullptr;
        std::optional<token> current_token_ = {};

        std::string_view input_;
    };

    std::ostream& operator<<(std::ostream& out, debug_print<lexer::sentinel> const& sentinel);

    std::ostream& operator<<(std::ostream& out, debug_print<lexer::iterator> const& iter);

    class lexer::iterator {
        friend std::ostream& operator<<(
            std::ostream& out, debug_print<lexer::iterator> const& iter);

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = noctern::token;
        using pointer = value_type const*;
        using reference = value_type const&;
        using iterator_category = std::input_iterator_tag;

        iterator() = default;

        iterator(lexer::sentinel)
            : iterator() {
        }

        explicit iterator(lexer& lexer)
            : lexer_ {&lexer} {
        }

        iterator& operator++() {
            lexer_->advance();
            return *this;
        }

        iterator& operator++(int) {
            return ++*this;
        }

        reference operator*() const {
            return lexer_->read();
        }

        pointer operator->() const {
            return &lexer_->read();
        }

        friend bool operator==(iterator const& lhs, iterator const& rhs) {
            if (lhs == lexer::sentinel {}) { return lexer::sentinel {} == rhs; }
            if (rhs == lexer::sentinel {}) { return lhs == lexer::sentinel {}; }

            return lhs.lexer_ == rhs.lexer_;
        }

        friend bool operator!=(iterator const& lhs, iterator const& rhs) {
            return !(lhs == rhs);
        }

        friend bool operator==(iterator const& lhs, lexer::sentinel) {
            return !lhs.has_next();
        }

        friend bool operator==(lexer::sentinel lhs, iterator const& rhs) {
            return rhs == lhs;
        }

        friend bool operator!=(iterator const& lhs, lexer::sentinel rhs) {
            return !(lhs == rhs);
        }

        friend bool operator!=(lexer::sentinel lhs, iterator const& rhs) {
            return !(lhs == rhs);
        }

    private:
        bool has_next() const {
            return lexer_ != nullptr && lexer_->has_next();
        }

        lexer* lexer_ = nullptr;
    };

    inline lexer::iterator lexer::begin() {
        return iterator {*this};
    }

    inline lexer::iterator lexer::end() const {
        return end_sentinel();
    }

    inline lexer::sentinel lexer::end_sentinel() const {
        return sentinel {};
    }
}
