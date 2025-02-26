#pragma once

#include "./tokenize.hpp"

#include <catch2/catch.hpp>
#include <string>

namespace Catch {
    template <>
    struct StringMaker<noctern::token> {
        static std::string convert(noctern::token token) {
            return std::string(stringify(token));
        }
    };
}
