#pragma once

#include "./tokenize.hpp"

#include <catch2/catch.hpp>
#include <string>

namespace Catch {
    template <>
    struct StringMaker<noctern::token_id> {
        static std::string convert(noctern::token_id token_id) {
            return std::string(stringify(token_id));
        }
    };
}
