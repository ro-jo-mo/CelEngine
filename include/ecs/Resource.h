#pragma once

#include "IResource.h"

#include <assert.h>
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

    // Null constructor
    Resource() = default;

    template<typename... Args>
    explicit Resource(std::in_place_t, Args&&... args)
        : resource(std::make_unique<T>(std::forward<Args>(args)...))
    {
    }

    using inner = T;

    T* get();

    T* operator->();

    T& operator*();

    template<typename... Args>
    void initialise(Args&&... args);

  protected:
    std::unique_ptr<T> resource;
};

template<typename T>
T*
Resource<T>::get()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return resource.get();
}

template<typename T>
T*
Resource<T>::operator->()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return resource.get();
}

template<typename T>
T&
Resource<T>::operator*()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return *resource;
}

template<typename T>
template<typename... Args>
void
Resource<T>::initialise(Args&&... args)
{
    resource = std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief A version of resource allowing parallel read / write access
 * Basic read / write locking system.
 * @tparam T
 */
template<typename T>
class ParallelResource : public IResource
{
  public:
    // Remove copy constructor
    ParallelResource(const ParallelResource&) = delete;
    ParallelResource& operator=(const ParallelResource&) = delete;

    // Null constructor
    ParallelResource() = default;

    template<typename... Args>
    explicit ParallelResource(std::in_place_t, Args&&... args)
        : resource(std::make_unique<T>(std::forward<Args>(args)...))
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

    template<typename... Args>
    void initialise(Args&&... args);

  protected:
    std::shared_mutex fullMutex;
    std::shared_mutex partialMutex;
    std::unique_ptr<T> resource;
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
    assert(resource != nullptr && "resource is accessed before initialisation");
    return { fullMutex, *resource };
}

template<typename T>
ParallelResource<T>::ReadGuard
ParallelResource<T>::read()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return { fullMutex, *resource };
}

template<typename T>
ParallelResource<T>::ReadGuard
ParallelResource<T>::partial_read()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return { partialMutex, *resource };
}

template<typename T>
ParallelResource<T>::AbsoluteGuard
ParallelResource<T>::absolute()
{
    assert(resource != nullptr && "resource is accessed before initialisation");
    return { fullMutex, partialMutex, *resource };
}

template<typename T>
template<typename... Args>
void
ParallelResource<T>::initialise(Args&&... args)
{
    resource = std::make_unique<T>(std::forward<Args>(args)...);
}

}