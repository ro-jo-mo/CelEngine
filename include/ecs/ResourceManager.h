#pragma once

#include <any>
#include "Resource.h"
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
  class ResourceManager {
  public:
    /**
     * @brief Initialise a new resource
     * @tparam T Resource type
     * @tparam Args Argument types
     * @param args Arguments to initialise resource with
     */
    template<typename T, typename... Args>
    void InsertResource(Args &&... args);

    /**
     * @brief Return resource
     * @tparam T Resource type
     * @return The resource
     */
    template<typename T>
    auto &GetResource();

  private:
    std::unordered_map<std::type_index, std::unique_ptr<IResource> > resources;
  };

  template<typename T, typename... Args>
  void ResourceManager::InsertResource(Args &&... args) {
    resources[typeid(Resource<T>)] = std::make_unique<Resource<T> >(std::forward<Args>(args)...);
  }

  template<typename T>
  auto &ResourceManager::GetResource() {
    const auto &ptr = resources[typeid(T)];
    return *static_cast<T *>(ptr.get());
  }
}
