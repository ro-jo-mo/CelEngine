#pragma once

#include <vector>

namespace Cel {

template<typename T>
struct Handle
{
    uint32_t index;

    bool operator==(const Handle& other) const { return index == other.index; }

    auto operator<=>(const Handle&) const = default;
};

}

template<typename T>
struct std::hash<Cel::Handle<T>>
{
    size_t operator()(const Cel::Handle<T>& h) const noexcept
    {
        return std::hash<uint32_t>{}(h.index);
    }
};
