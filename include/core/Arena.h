#pragma once
#include <cstdlib>

namespace Cel {

// https://en.cppreference.com/cpp/named_req/Allocator

// Very basic arena allocator
// Does not call destructors on reset()
// Will reallocate if capacity is not large enough
// To avoid invalidation we return "ArenaPointer"
class Arena
{
  public:
    struct ArenaPointer
    {
        [[nodiscard]] void* get() const;

        char** basePtr;
        size_t offset;
    };

    explicit Arena(size_t defaultAllocSize = 2 << 16);

    ArenaPointer allocate(size_t size, size_t align);

    void reset();

  private:
    static char* create_allocation(size_t size);

    void reallocate();

    char* allocation;
    size_t allocationSize;
    size_t upTo = 0;
};

}