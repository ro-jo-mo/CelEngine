#pragma once
#include <cstdint>

namespace Cel {

using ComponentId = uint32_t;

using ResourceId = uint32_t;

using QueryId = uint32_t;

namespace Detail {

inline ComponentId&
get_component_counter()
{
    static uint32_t counter = 0;
    return counter;
}

inline ResourceId&
get_resource_counter()
{
    static uint32_t counter = 0;
    return counter;
}

inline ResourceId&
get_query_counter()
{
    static uint32_t counter = 0;
    return counter;
}

}

// TODO: I might have to handle constness here
template<typename T>
ComponentId
component_id()
{
    static auto id = Detail::get_component_counter()++;
    return id;
}

template<typename T>
ResourceId
resource_id()
{
    static auto id = Detail::get_resource_counter()++;
    return id;
}

template<typename T>
QueryId
query_id()
{
    static auto id = Detail::get_query_counter()++;
    return id;
}

}