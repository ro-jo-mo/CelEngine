#pragma once
#include <optional>

namespace Cel {

// A type used to specify optional retrieval for a component in a query
// Simple wrapper of std::optional with a distinct type
template<typename T>
class Optional : std::optional<T>
{
    using std::optional<T>::optional;  // inherit all constructors
    using std::optional<T>::operator=; // inherit assignment

    explicit Optional(const std::optional<T>& val)
        : std::optional<T>(val)
    {
    }
    explicit Optional(std::optional<T>&& val)
        : std::optional<T>(std::move(val))
    {
    }
};

template<typename T>
struct IsOptional : std::false_type
{};

template<typename T>
struct IsOptional<Optional<T>> : std::true_type
{};

}