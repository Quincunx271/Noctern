#pragma once

#include <algorithm>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

#include "noctern/meta.hpp"

#define NOCTERN_ENUM_X_INTROSPECT(name)                                                            \
    case name: {                                                                                   \
        constexpr std::string_view name_str = #name;                                               \
        return std::invoke(std::forward<Fn>(fn), val<name>, name_str);                             \
    }

namespace noctern {
    template <typename Enum>
    constexpr auto to_underlying(Enum e) {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    namespace enum_internal {
        struct access;
    }

    // An elaborated `enum` which supports additional features (via ADL):
    //
    //   std::string_view stringify(Enum);
    //   enum_values(type<Enum>, [](Enum...) { ... });
    //   enum_switch(Enum, [](val_t<Enum>) { ... });
    //
    // Usage:
    //
    //   struct _my_enum_wrapper {
    //     enum class my_enum {
    //       value_a, value_b,
    //     };
    //
    //   private:
    //     friend enum_mixin;
    //
    //     template <typename Fn>
    //     friend constexpr decltype(auto) switch_introspect(my_enum e, Fn&& fn) {
    //         switch (e) {
    //           using enum my_enu;
    //           // If using X macros, see NOCTERN_ENUM_X_INTROSPECT.
    //         case value_a: return fn(val<value_a>, "value_a");
    //         case value_b: return fn(val<value_b>, "value_b");
    //         }
    //     }

    //     template <typename Fn>
    //     friend constexpr decltype(auto) introspect(type_t<my_enum>, Fn&& fn) {
    //         using enum my_enum;
    //         return std::invoke(std::forward<Fn>(fn), value_a, value_b);
    //     }
    //   };
    //   using my_enum = _my_enum_wrapper::my_enum;
    struct enum_mixin {
    private:
        // We need this extra step to work around a Clang "bug" (not-yet-implemented functionality).

        friend enum_internal::access;

        template <typename Enum, typename Fn>
        static constexpr decltype(auto) do_switch_introspect(Enum e, Fn&& fn) {
            return switch_introspect(e, std::forward<Fn>(fn));
        }

        template <typename Enum, typename Fn>
        static constexpr decltype(auto) do_introspect(type_t<Enum>, Fn&& fn) {
            return introspect(type<Enum>, std::forward<Fn>(fn));
        }
    };

    namespace enum_internal {
        struct access : enum_mixin {
            using enum_mixin::do_introspect;
            using enum_mixin::do_switch_introspect;
        };
    }

    /////
    // The key functions. These are not hidden friends to work around a Clang bug.
    
    template <typename Enum>
        requires std::is_enum_v<Enum>
    constexpr std::string_view stringify(Enum e) {
        return enum_internal::access::do_switch_introspect(
            e, []<Enum e>(val_t<e>, std::string_view str) { return str; });
    }

    template <typename Enum, typename Fn>
        requires std::is_enum_v<Enum>
    constexpr decltype(auto) enum_switch(Enum e, Fn&& fn) {
        return enum_internal::access::do_switch_introspect(
            e, [&]<Enum e>(val_t<e>, std::string_view) -> decltype(auto) {
                return std::invoke(std::forward<Fn>(fn), val<e>);
            });
    }

    template <typename Enum, typename Fn>
        requires std::is_enum_v<Enum>
    constexpr decltype(auto) enum_values(type_t<Enum>, Fn&& fn) {
        return enum_internal::access::do_introspect(
            type<Enum>, [&]<Enum... es>(val_t<es>...) -> decltype(auto) {
                return std::invoke(std::forward<Fn>(fn), val<es>...);
            });
    }
    
    // End key functions.
    /////

    template <typename Enum>
        requires std::is_enum_v<Enum>
    constexpr size_t enum_count(type_t<Enum>) {
        return enum_values(type<Enum>, [](auto... enums) { return sizeof...(enums); });
    }

    template <typename Enum>
        requires std::is_enum_v<Enum>
    constexpr size_t enum_min(type_t<Enum>) {
        return enum_values(type<Enum>,
            []<Enum... es>(val_t<es>...) { return std::min({noctern::to_underlying(es)...}); });
    }

    template <typename Enum>
        requires std::is_enum_v<Enum>
    constexpr size_t enum_max(type_t<Enum>) {
        return enum_values(type<Enum>,
            []<Enum... es>(val_t<es>...) { return std::max({noctern::to_underlying(es)...}); });
    }
}
