#pragma once

#include "Resource.h"
#include <any>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>

namespace Cel {
/**
 * @brief Stores and retrieves resources
 * In this engine resources are treated as singleton instances of an object.
 * Currently they're assumed to be thread safe (lazy me)
 * At a later date I will likely change this?
 */
class ResourceManager
{
  public:
    /**
     * @brief Initialise a new resource
     * @tparam T Resource type
     * @tparam Args Argument types
     * @param args Arguments to initialise resource with
     */
    template<typename T, typename... Args>
    Resource<T>& InsertResource(Args&&... args);

    template<typename T>
    Resource<T>& InsertResource(T resource);

    /**
     * @brief Return resource
     * @tparam T Resource type
     * @return The resource
     */
    template<typename T>
    Resource<T>& GetResource();

  private:
    std::unordered_map<std::type_index, std::unique_ptr<IResource>> resources;
};

template<typename T, typename... Args>
Resource<T>&
ResourceManager::InsertResource(Args&&... args)
{
    assert(!resources.contains(typeid(Resource<T>)));

    resources[typeid(Resource<T>)] =
        std::make_unique<Resource<T>>(std::forward<Args>(args)...);
    return GetResource<T>();
}

template<typename T>
Resource<T>&
ResourceManager::InsertResource(T resource)
{
    assert(!resources.contains(typeid(Resource<T>)));

    resources[typeid(Resource<T>)] = std::make_unique<Resource<T>>(resource);
    return GetResource<T>();
}

template<typename T>
Resource<T>&
ResourceManager::GetResource()
{
    assert(resources.contains(typeid(Resource<T>)));

    const auto& ptr = resources[typeid(Resource<T>)];
    return *static_cast<Resource<T>*>(ptr.get());
}
}
