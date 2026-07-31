#pragma once
#include <vector>

namespace Cel::Common {
// Just a convenient vector wrapper that grows automatically when indexed out of range
// Effectively used as a uint map to T
template<typename T>
class GrowVector
{
  public:
    void push_back(T t);
    size_t size() const;
    T operator[](int i);

  private:
    std::vector<T> internal;
};

template<typename T>
void
GrowVector<T>::push_back(T t)
{
    internal.push_back(t);
}

template<typename T>
size_t
GrowVector<T>::size() const
{
    return internal.size();
}

template<typename T>
T
GrowVector<T>::operator[](int i)
{
    if (i >= internal.size()) {
        // geometric growth
        internal.resize(2 * internal.size());
    }

    return internal[i];
}

}