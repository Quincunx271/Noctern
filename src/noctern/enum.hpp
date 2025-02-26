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

#define NOCTERN_ENUM_MAKE_MIXIN_FORWARDS(name)                                                     \
    /* Converts an enum to string. */                                                              \
    friend constexpr std::string_view stringify(name _enum_value) {                                \
        return ::noctern::enum_mixin::stringify(_enum_value);                                      \
    }                                                                                              \
                                                                                                   \
    /* Lifts a switch on enumeration values into template parameters. */                           \
    template <typename TheFnType>                                                                  \
    friend constexpr decltype(auto) enum_switch(name _enum_value, TheFnType&& _fn) {               \
        return ::noctern::enum_mixin::enum_switch(_enum_value, std::forward<TheFnType>(_fn));      \
    }                                                                                              \
                                                                                                   \
    /* Visits all the enumeration values. */                                                       \
    template <typename TheFnType>                                                                  \
    friend constexpr decltype(auto) enum_values(type_t<name>, TheFnType&& _fn) {                   \
        return ::noctern::enum_mixin::enum_values(type<name>, std::forward<TheFnType>(_fn));       \
    }

namespace noctern {
    template <typename Enum>
    constexpr auto to_underlying(Enum e) {
        return static_cast<std::underlying_type_t<Enum>>(e);
    }

    // An elaborated `enum` which supports additional features (via ADL):
    //
    //   std::string_view stringify(Enum);
    //   enum_values(type<Enum>, [](Enum...) { ... });
    //   enum_switch(Enum, [](val_t<Enum>) { ... });
    struct enum_mixin {
        template <typename Enum>
            requires std::is_enum_v<Enum>
        static constexpr std::string_view stringify(Enum e) {
            return switch_introspect(e, []<Enum e>(val_t<e>, std::string_view str) { return str; });
        }

        template <typename Enum, typename Fn>
            requires std::is_enum_v<Enum>
        static constexpr decltype(auto) enum_switch(Enum e, Fn&& fn) {
            return switch_introspect(e, [&]<Enum e>(val_t<e>, std::string_view) -> decltype(auto) {
                return std::invoke(std::forward<Fn>(fn), val<e>);
            });
        }

        template <typename Enum, typename Fn>
            requires std::is_enum_v<Enum>
        static constexpr decltype(auto) enum_values(type_t<Enum>, Fn&& fn) {
            return introspect(type<Enum>, [&]<Enum... es>(val_t<es>...) -> decltype(auto) {
                return std::invoke(std::forward<Fn>(fn), val<es>...);
            });
        }
    };

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
