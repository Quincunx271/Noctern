#pragma once

#include <noctern/data/ast.hpp>
#include <noctern/data/token.hpp>
#include <span>

namespace noctern {
    ast::file parse(std::span<token const> tokens);
}
