#pragma once

#include "Query.h"
#include "QueryManager.h"
#include "ResourceManager.h"
#include "Helpers.h"
#include "Locks.h"

namespace Cel {
    class SystemAllocator {
    public:
        SystemAllocator(ResourceManager &resource_manager, QueryManager &query_manager)
            : resourceManager(resource_manager),
              queryManager(query_manager) {
        }

        template<typename T>
        T &Register();

    private:
        std::vector<std::type_index> registeredResources;
        std::vector<std::type_index> registeredQueries;
        ResourceManager &resourceManager;
        QueryManager &queryManager;
        Locks locks;
    };

    template<typename T>
    T &SystemAllocator::Register() {
        locks.Register<T>();
        if constexpr (IsQuery<T>::value) {
            registeredQueries.push_back(std::type_index(typeid(T)));
            return queryManager.GetQuery<T>();
        } else if constexpr (IsResource<T>::value) {
            registeredResources.push_back(std::type_index(typeid(T)));
            return resourceManager.GetResource<T>();
        } else {
            static_assert(alwaysFalse<T>, "A system can only request resources and queries!");
            return T();
        }
    }
}
