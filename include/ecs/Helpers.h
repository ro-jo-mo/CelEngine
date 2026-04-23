#pragma once

#include "Query.h"
#include "Resource.h"

namespace Cel {
    /**
     * @defgroup Helper types
     * @{
     */
    template<typename... Ts>
    struct IsQuery : std::false_type {
    };

    template<typename... Ts>
    struct IsQuery<Query<Ts...> > : std::true_type {
    };

    template<typename T>
    struct IsResource : std::false_type {
    };

    template<typename T>
    struct IsResource<Resource<T> > : std::true_type {
    };

    template<typename... T>
    constexpr bool alwaysFalse = false;
    /** @} */
}
