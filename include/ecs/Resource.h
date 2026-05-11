#pragma once
#include "IResource.h"
#include <utility>

namespace Cel {
/**
 * Wrapper class for resources
 * @tparam T Resource type
 */
template<typename T>
class Resource : public IResource
{
  public:
    // Remove copy construtor
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    template<typename... Args>
    explicit Resource(Args&&... args)
        : resource(T(std::forward<Args>(args)...))
    {
    }

    using inner = T;

    T* operator->();

    T operator*() const;

  private:
    T resource;
};

template<typename T>
T*
Resource<T>::operator->()
{
    return &resource;
}

template<typename T>
T
Resource<T>::operator*() const
{
    return resource;
}
}
