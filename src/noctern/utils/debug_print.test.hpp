#pragma once

#include <ostream>
#include <sstream>
#include <type_traits>
#include <utility>

#include <catch2/catch.hpp>

#include <noctern/utils/debug_print.hpp>

namespace Catch {
    template <typename T>
    struct StringMaker<T,
        std::void_t<decltype(
            std::declval<std::ostream&>() << noctern::debug_print(std::declval<T const&>()))>> {
        static std::string convert(T const& value) {
            std::ostringstream out;
            out << noctern::debug_print(value);
            return out.str();
        }
    };
}
