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

template<typename T>
ComponentId
_component_id()
{
    static auto id = get_component_counter()++;
    return id;
}

template<typename T>
ResourceId
_resource_id()
{
    static auto id = get_resource_counter()++;
    return id;
}

template<typename T>
QueryId
_query_id()
{
    static auto id = get_query_counter()++;
    return id;
}

}

template<typename T>
ComponentId
component_id()
{
    return Detail::_component_id<std::remove_const_t<T>>();
}

template<typename T>
ResourceId
resource_id()
{
    return Detail::_resource_id<std::remove_const_t<T>>();
}

template<typename T>
QueryId
query_id()
{
    return Detail::_query_id<std::remove_const_t<T>>();
}

}