#pragma once

#include <chrono>
#include <fmt/base.h>
#include <stdexcept>
#include <thread>

namespace Cel {
// For some very annoying reason, my runtime deletes the message from
// runtime_error As such I also print to stderr
inline void
ThrowError(const char* message)
{
    fmt::println(stderr, "{}", message);
    throw std::runtime_error(message);
};

void inline SleepPrint(const char* string)
{
    fmt::println("{}", string);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

}