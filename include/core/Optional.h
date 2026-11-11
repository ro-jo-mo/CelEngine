#pragma once
#include <optional>

namespace Cel {

// A type used to specify optional retrieval for a component in a query
// Simple wrapper of std::optional with a distinct type
template<typename T>
struct Optional
{
    using type = T;
};

template<typename T>
constexpr bool is_optional = false;

template<typename T>
constexpr bool is_optional<Optional<T>> = true;

}