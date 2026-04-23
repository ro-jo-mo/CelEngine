#pragma once
#include "IResource.h"

namespace Cel {
    /**
     * Wrapper class for resources
     * @tparam T Resource type
     */
    template<typename T>
    class Resource : IResource {
    public:
        template<typename... Args>
        explicit Resource(Args &&... args) : resource(T(std::forward<Args>(args)...)) {
        }

        using inner = T;

        T *operator ->();

    private:
        T resource;
    };

    template<typename T>
    T *Resource<T>::operator->() {
        return &resource;
    }
}
