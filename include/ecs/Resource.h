#pragma once
#include "IResource.h"

#include <mutex>
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
    // Remove copy constructor
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;

    template<typename... Args>
    explicit Resource(Args&&... args)
        : resource(T(std::forward<Args>(args)...))
    {
    }

    using inner = T;

    T* operator->();

    T& operator*();

  protected:
    T resource;
};

template<typename T>
T*
Resource<T>::operator->()
{
    return &resource;
}

template<typename T>
T&
Resource<T>::operator*()
{
    return resource;
}

/**
 * @brief A version of resource allowing parallel read / write access
 * Allows two or more systems to access the same resource in parallel even if
 * they're writing to it
 * @tparam T
 */
template<typename T>
class ParallelResource : IResource
{
  public:
    // Remove copy constructor
    ParallelResource(const ParallelResource&) = delete;
    ParallelResource& operator=(const ParallelResource&) = delete;

    template<typename... Args>
    explicit ParallelResource(Args&&... args)
        : resource(T(std::forward<Args>(args)...))
    {
    }

    using inner = T;

    class Guard
    {
      public:
        T* operator->();

        T& operator*();

      private:
        std::lock_guard<std::mutex> guard;
        T& ref;
    };

    Guard lock();

  protected:
    std::mutex mutex;
    T resource;
};

template<typename T>
T*
ParallelResource<T>::Guard::operator->()
{
    return &ref;
}

template<typename T>
T&
ParallelResource<T>::Guard::operator*()
{
    return ref;
}

template<typename T>
ParallelResource<T>::Guard
ParallelResource<T>::lock()
{
    return Guard{ mutex, resource };
}

}
