#pragma once

#include <concepts>

// Helpers for specifying in/out/inout parameters with odd lifetime requirements.
//
// If the lifetime requirements are standard (i.e. "live until function ends"), this isn't needed,
// but if we're holding a reference onto the parameter, we should pass the parameter via one of
// these aliases, constructed via CTAD.
//
// Example usage:
//
//   void do_something(int x, int y, inout<vector<int>> storage) {
//       do_something_else(x, inout(storage));
//       do_something_else(y, inout(storage));
//   }

namespace noctern {
    // Marks inout parameters with odd lifetime requirements.
    template <typename T>
    class inout {
    public:
        explicit constexpr inout(T& ref)
            : ref_(&ref) {
        }

        template <typename U>
            requires std::convertible_to<U&, T&>
        explicit constexpr inout(inout<U>& rhs)
            : ref_(rhs.ref_) {
        }

        // This is not a value type.
        constexpr inout(const inout&) = delete;

        T& operator*() {
            return *ref_;
        }

        T* operator->() {
            return ref_;
        }

    private:
        T* ref_;
    };

    // Marks input parameters with odd lifetime requirements.
    template <typename T>
    class in {
    public:
        explicit constexpr in(T& ref)
            : ref_(&ref) {
        }

        template <typename U>
            requires std::convertible_to<U&, T&>
        explicit constexpr in(in<U>& rhs)
            : ref_(rhs.ref_) {
        }

        // This is not a value type.
        constexpr in(const in&) = delete;

        T& operator*() {
            return *ref_;
        }

        T* operator->() {
            return ref_;
        }

    private:
        T* ref_;
    };

    // Marks output parameters with odd lifetime requirements.
    template <typename T>
    class out {
    public:
        explicit constexpr out(T& ref)
            : ref_(&ref) {
        }

        template <typename U>
            requires std::convertible_to<U&, T&>
        explicit constexpr out(out<U>& rhs)
            : ref_(rhs.ref_) {
        }

        // This is not a value type.
        constexpr out(const out&) = delete;

        T& operator*() {
            return *ref_;
        }

        T* operator->() {
            return ref_;
        }

    private:
        T* ref_;
    };

    static_assert(!std::copy_constructible<inout<int>>);
    static_assert(!std::copy_constructible<in<int>>);
    static_assert(!std::copy_constructible<out<int>>);
}
