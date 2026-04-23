#pragma once

#include "ComponentsManager.h"
#include <memory>

namespace Cel {
    /**
     * @brief Dummy interface for query polymorphism
     */
    class IQuery {
    public:
        virtual ~IQuery() = default;

        /**
         * @brief A method to update the set of entities this query accesses
         */
        virtual void UpdateQuery() = 0;
    };
}
