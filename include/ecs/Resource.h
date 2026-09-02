#pragma once

#include "IResource.h"

#include <shared_mutex>
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
 * Basic read / write locking system.
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

    class WriteGuard
    {
      public:
        WriteGuard(auto& mutex, auto& ref)
            : guard(mutex)
            , ref(ref)
        {
        }

        T* operator->();

        T& operator*();

      private:
        std::unique_lock<std::shared_mutex> guard;
        T& ref;
    };

    class ReadGuard
    {
      public:
        ReadGuard(auto& mutex, auto& ref)
            : guard(mutex)
            , ref(ref)
        {
        }

        const T* operator->();

        const T& operator*();

      private:
        std::shared_lock<std::shared_mutex> guard;
        const T& ref;
    };

    WriteGuard write();

    ReadGuard read();

    /**
     * In the case of PassServer in particular, the const access functionality
     * could be an entirely separate resource. That is to say there is no read
     * write conflicts, and using a read lock would simply be wasteful.
     *
     * This should obviously not be used unless you are already familiar with
     * the resources inner workings, and have firm guarantees about the
     * functionality
     * @return
     */
    const T& illegal();

  protected:
    std::shared_mutex mutex;
    T resource;
};

template<typename T>
T*
ParallelResource<T>::WriteGuard::operator->()
{
    return &ref;
}

template<typename T>
T&
ParallelResource<T>::WriteGuard::operator*()
{
    return ref;
}

template<typename T>
const T*
ParallelResource<T>::ReadGuard::operator->()
{
    return &ref;
}

template<typename T>
const T&
ParallelResource<T>::ReadGuard::operator*()
{
    return ref;
}

template<typename T>
ParallelResource<T>::WriteGuard
ParallelResource<T>::write()
{
    return { mutex, resource };
}

template<typename T>
ParallelResource<T>::ReadGuard
ParallelResource<T>::read()
{
    return { mutex, resource };
}

template<typename T>
const T&
ParallelResource<T>::illegal()
{
    return resource;
}
}
