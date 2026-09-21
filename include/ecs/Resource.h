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

    class AbsoluteGuard : public WriteGuard
    {
      public:
        AbsoluteGuard(auto& mutex, auto& partial, auto& ref)
            : WriteGuard(mutex, ref)
            , partialGuard(partial)
        {
        }

      private:
        std::unique_lock<std::shared_mutex> partialGuard;
    };

    WriteGuard write();

    ReadGuard read();

    /**
     * In the case where we have a firm guarantee the data we're reading is
     * different from the data that could be written to, we can use a partial
     * read.
     *
     * This really only applies to a case like the pass server, where
     * write access is required for cmd buffers, but never resource access.
     *
     * This only blocks the absolute lock.
     *
     */
    ReadGuard partial_read();

    /**
     * A fully blocking lock. Sort of like joining the threads.
     *
     * @return
     */
    AbsoluteGuard absolute();

  protected:
    std::shared_mutex fullMutex;
    std::shared_mutex partialMutex;
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
    return { fullMutex, resource };
}

template<typename T>
ParallelResource<T>::ReadGuard
ParallelResource<T>::read()
{
    return { fullMutex, resource };
}

template<typename T>
ParallelResource<T>::ReadGuard
ParallelResource<T>::partial_read()
{
    return { partialMutex, resource };
}

template<typename T>
ParallelResource<T>::AbsoluteGuard
ParallelResource<T>::absolute()
{
    return { fullMutex, partialMutex, resource };
}

}
