#pragma once

#include <any>
#include "Resource.h"
#include <memory>
#include <typeindex>
#include <unordered_map>


namespace Cel {
  /**
   *
   */
  class ResourceManager {
  public:
    template<typename T, typename... Args>
    void InsertResource(Args &&... args);

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
