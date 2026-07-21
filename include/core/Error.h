#pragma once

#include <chrono>
#include <fmt/base.h>
#include <fmt/format.h>
#include <stdexcept>
#include <thread>

namespace Cel {
// For some very annoying reason, my runtime deletes the message from
// runtime_error As such I also print to stderr
template<typename... T>
inline void
throw_error(fmt::format_string<T...> fmt, const T&&... args)
{
    auto message = fmt::vformat(fmt.str, fmt::vargs<T...>{ { args... } });

    fmt::println(stderr, "{}", message);
    throw std::runtime_error(message);
}

void inline sleep_print(const char* string)
{
    fmt::println("{}", string);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

}