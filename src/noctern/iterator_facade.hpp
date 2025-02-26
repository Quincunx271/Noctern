#pragma once

#include <iterator>
#include <type_traits>

#include "noctern/meta.hpp"

namespace noctern {
    namespace iterator_facade_internal {
        // Helper to compute the difference_type.
        template <typename Self, int N = 0>
        constexpr auto diff() {
            if constexpr (requires { typename Self::difference_type; }) {
                return typename Self::difference_type {N};
            } else {
                return std::ptrdiff_t {N};
            }
        }

        template <typename Self>
        using diff_type = decltype(diff<Self>());

        template <typename Self, typename Diff>
        concept is_diff_type = std::convertible_to<Diff, diff_type<Self>>;

        template <typename Self>
        concept can_decrement = requires(Self self) { self.advance(val<diff<Self, -1>()>); };

        template <typename Self>
        concept can_advance
            = requires(Self self, const diff_type<Self> diff) { self.advance(diff); };

        template <typename Self>
        concept can_compute_distance = requires(const Self lhs, const Self rhs) {
            { lhs.distance(rhs) } -> std::convertible_to<diff_type<Self>>;
        };

        template <typename Self>
        concept can_compute_equal_to = requires(const Self lhs, const Self rhs) {
            { lhs.equal_to(rhs) } -> std::convertible_to<bool>;
        };
    }

    template <typename Derived>
    class iterator_facade {
    public:
        decltype(auto) operator*() const {
            return self().read();
        }

        template <typename Diff>
        decltype(auto) operator[](Diff offset) const
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            return *(self() + iterator_facade_internal::diff_type<Derived> {offset});
        }

        auto operator->() const
            requires(std::is_reference_v<decltype(**this)>)
        {
            return std::addressof(**this);
        }

        Derived& operator++() {
            self().advance(val<1>);
            return self();
        }

        Derived operator++(int) {
            Derived cpy = self();
            ++*this;
            return cpy;
        }

        template <typename Diff>
        Derived& operator+=(Diff diff)
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            self().advance(diff);
            return self();
        }

        template <typename Diff>
        friend Derived operator+(const Derived& self, Diff diff)
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            Derived cpy = self;
            cpy += diff;
            return cpy;
        }

        template <typename Diff>
        friend Derived operator+(Diff diff, const Derived& self)
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            return self + diff;
        }

        Derived& operator--()
            requires iterator_facade_internal::can_decrement<Derived>
        {
            self().advance(val<-1>);
            return self();
        }

        Derived operator--(int)
            requires iterator_facade_internal::can_decrement<Derived>
        {
            Derived cpy = self();
            self().advance(val<-1>);
            return cpy;
        }

        template <typename Diff>
        Derived& operator-=(Diff diff)
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            return *this += -diff;
        }

        template <typename Diff>
        friend Derived operator-(const Derived& self, Diff diff)
            requires(iterator_facade_internal::is_diff_type<Derived, Diff>
                && iterator_facade_internal::can_advance<Derived>)
        {
            Derived cpy = self;
            cpy -= diff;
            return cpy;
        }

        friend auto operator-(const Derived& lhs, const Derived& rhs)
            requires(iterator_facade_internal::can_compute_distance<Derived>)
        {
            return lhs.distance(rhs);
        }

        friend bool operator==(const Derived& lhs, const Derived& rhs)
            requires(iterator_facade_internal::can_compute_equal_to<Derived>)
        {
            return lhs.equal_to(rhs);
        }

        friend bool operator==(const Derived& lhs, const Derived& rhs)
            requires(iterator_facade_internal::can_compute_distance<Derived>
                && !iterator_facade_internal::can_compute_equal_to<Derived>)
        {
            return lhs.distance(rhs) == 0;
        }

        friend auto operator<=>(const Derived& lhs, const Derived& rhs)
            requires(iterator_facade_internal::can_compute_distance<Derived>)
        {
            return lhs.distance(rhs) <=> 0;
        }

    private:
        Derived& self() {
            return static_cast<Derived&>(*this);
        }

        const Derived& self() const {
            return static_cast<const Derived&>(*this);
        }
    };
}

template <typename Iter>
    requires std::derived_from<Iter, noctern::iterator_facade<Iter>>
struct std::iterator_traits<Iter> {
    using reference = decltype(*std::declval<const Iter&>());

    using pointer = decltype([](const Iter& iter) {
        if constexpr (requires { iter.operator->(); }) {
            return iter.operator->();
        } else {
            return;
        }
    }(std::declval<const Iter&>()));

    using difference_type = noctern::iterator_facade_internal::diff_type<Iter>;

    using value_type = decltype([](const Iter&) {
        if constexpr (requires { typename Iter::value_type; }) {
            return noctern::type<typename Iter::value_type>;
        } else {
            return noctern::type<std::remove_cvref_t<reference>>;
        }
    }(std::declval<const Iter&>()))::type;

    using iterator_category = decltype([](const Iter& c, Iter& m) {
        if constexpr (!std::is_reference_v<decltype(*c)>) {
            return std::input_iterator_tag {};
        } else if constexpr (requires { typename Iter::iterator_category; }) {
            return typename Iter::iterator_category {};
        } else if constexpr (requires {
                                 { c - c } -> std::convertible_to<difference_type>;
                                 { m + difference_type {2} } -> std::same_as<Iter>;
                             }) {
            return std::random_access_iterator_tag {};
        } else if constexpr (requires {
                                 { --m } -> std::same_as<Iter&>;
                             }) {
            return std::bidirectional_iterator_tag {};
        } else if constexpr (std::is_reference_v<decltype(*c)>) {
            return std::forward_iterator_tag {};
        } else {
            return std::input_iterator_tag {};
        }
    }(std::declval<const Iter&>(), std::declval<Iter&>()));

    using iterator_concept = decltype([](const Iter& c, Iter& m) {
        if constexpr (requires { typename Iter::iterator_concept; }) {
            return typename Iter::iterator_concept {};
        } else if constexpr (requires { typename Iter::iterator_category; }) {
            return typename Iter::iterator_category {};
        } else if constexpr (requires {
                                 { c - c } -> std::convertible_to<difference_type>;
                                 { m + difference_type {2} } -> std::same_as<Iter>;
                             }) {
            return std::random_access_iterator_tag {};
        } else if constexpr (requires {
                                 { --m } -> std::same_as<Iter&>;
                             }) {
            return std::bidirectional_iterator_tag {};
        } else {
            return std::forward_iterator_tag {};
        }
    }(std::declval<const Iter&>(), std::declval<Iter&>()));
};