#pragma once
#include <fmt/base.h>
#include <stdexcept>

namespace Cel {
inline void
ThrowError(const char* message)
{
    fmt::println(stderr, "{}", message);
    throw std::runtime_error(message);
};
}