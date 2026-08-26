#pragma once

#include "Resource.h"
#include "core/Error.h"

#include <any>
#include <cassert>
#include <fmt/base.h>
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
    Resource<T>& insert_resource(Args&&... args);

    template<typename T>
    Resource<T>& insert_resource(T resource);

    template<typename T, typename... Args>
    ParallelResource<T>& insert_parallel_resource(Args&&... args);

    template<typename T>
    ParallelResource<T>& insert_parallel_resource(T resource);

    /**
     * @brief Return resource
     * @tparam T Resource type
     * @return The resource
     */
    template<typename T>
    Resource<T>& get_resource();

    template<typename T>
    ParallelResource<T>& get_parallel_resource();

  private:
    std::unordered_map<std::type_index, std::unique_ptr<IResource>> resources;
    std::unordered_map<std::type_index, std::unique_ptr<IResource>>
        parallelResources;
};

template<typename T, typename... Args>
Resource<T>&
ResourceManager::insert_resource(Args&&... args)
{
    using Type = Resource<std::remove_const_t<T>>;
    const std::type_index id = typeid(Type);

    if (resources.contains(id)) {
        throw_error("inserted resource already exists", typeid(T).name());
    }

    resources[id] = std::make_unique<Type>(std::forward<Args>(args)...);

    return get_resource<T>();
}

template<typename T>
Resource<T>&
ResourceManager::insert_resource(T resource)
{
    using Type = Resource<std::remove_const_t<T>>;
    const std::type_index id = typeid(Type);

    if (resources.contains(id)) {
        throw_error("inserted resource already exists", typeid(T).name());
    }
    resources[id] = std::make_unique<Type>(resource);

    return get_resource<T>();
}

template<typename T, typename... Args>
ParallelResource<T>&
ResourceManager::insert_parallel_resource(Args&&... args)
{
    using Type = ParallelResource<std::remove_const_t<T>>;
    const std::type_index id = typeid(Type);

    if (parallelResources.contains(id)) {
        throw_error("inserted parallel resource already exists",
                    typeid(T).name());
    }

    parallelResources[typeid(ParallelResource<T>)] =
        std::make_unique<Type>(std::forward<Args>(args)...);

    return get_resource<T>();
}

template<typename T>
ParallelResource<T>&
ResourceManager::insert_parallel_resource(T resource)
{
    using Type = ParallelResource<std::remove_const_t<T>>;
    const std::type_index id = typeid(Type);

    if (parallelResources.contains(id)) {
        throw_error("inserted parallel resource already exists",
                    typeid(T).name());
    }
    parallelResources[id] = std::make_unique<Type>(resource);

    return get_resource<T>();
}

template<typename T>
Resource<T>&
ResourceManager::get_resource()
{
    using Type = Resource<std::remove_const_t<T>>;
    std::type_index id = typeid(Type);

    if (!resources.contains(id)) {
        throw_error("requested resource does not exist yet {}",
                    typeid(Type).name());
    }

    const auto& ptr = resources.at(id);

    return *static_cast<Type*>(ptr.get());
}

template<typename T>
ParallelResource<T>&
ResourceManager::get_parallel_resource()
{
    using Type = ParallelResource<std::remove_const_t<T>>;
    const std::type_index id = typeid(Type);

    if (!resources.contains(id)) {
        throw_error("requested parallel resource does not exist yet {}",
                    typeid(Type).name());
    }

    const auto& ptr = parallelResources.at(id);

    return *static_cast<Type*>(ptr.get());
}

}
