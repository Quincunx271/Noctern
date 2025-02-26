#pragma once

#include <cstddef>
#include <string_view>

namespace noctern {
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

    template <typename T>
    struct type_t {
        using type = T;
    };

    template <typename T>
    inline constexpr auto type = type_t<T> {};

    template <auto V>
    struct val_t {
        static constexpr auto value = V;
    };

    template <auto V>
    inline constexpr auto val = val_t<V> {};

    template <fixed_string S>
    inline constexpr auto val<S> = val_t<S> {};
}