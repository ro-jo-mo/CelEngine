#pragma once
#include <fmt/base.h>
#include <stdexcept>

namespace Cel {
// For some very annoying reason, my runtime deletes the message from runtime_error
// As such I also print to stderr
inline void
ThrowError(const char* message)
{
    fmt::println(stderr, "{}", message);
    throw std::runtime_error(message);
};
}