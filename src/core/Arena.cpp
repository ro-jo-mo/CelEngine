#include "core/Arena.h"

#include <cstring>

Cel::Arena::Arena(const size_t defaultAllocSize)
    : allocation(create_allocation(defaultAllocSize))
    , allocationSize(defaultAllocSize)
{
}

void*
Cel::Arena::ArenaPointer::get() const
{
    return *basePtr + offset;
}

Cel::Arena::ArenaPointer
Cel::Arena::allocate(const size_t size, const size_t align)
{
    // I think this might be correct formula?
    const size_t start = ((align - 1 + upTo) / align) * align;

    upTo = start + size;

    while (upTo >= allocationSize) {
        reallocate();
    }

    return { .basePtr = &allocation, .offset = start };
}

void
Cel::Arena::reset()
{
    upTo = 0;
}

void
Cel::Arena::reallocate()
{
    auto newSize = allocationSize * 2;
    auto newAlloc = create_allocation(newSize);

    // Copy data and then free
    memcpy(newAlloc, allocation, allocationSize);
    free(allocation);

    // Set data for the new allocation
    allocationSize = newSize;
    allocation = newAlloc;
}

char*
Cel::Arena::create_allocation(const size_t size)
{
    return static_cast<char*>(std::malloc(size));
}
