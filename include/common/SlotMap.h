#pragma once
#include "core/Error.h"

#include <stdexcept>
#include <vector>
namespace Cel::Common {
// Based on Allan Deutsch talk from C++ Now 2017
template<typename T>
class SlotMap
{
  public:
    SlotMap() { AddFreeListSLots(); }

    // Index points to the data array
    // Generation ensures a slot is still valid
    struct Slot
    {
        uint32_t index;
        uint32_t generation = 0;
    };

    Slot Push(T value);
    void Remove(Slot slot);
    T Get(Slot slot);
    bool Valid(Slot slot);

  private:
    void AddFreeListSLots();

    std::vector<Slot> indices;
    std::vector<T> data;
    std::vector<uint32_t> erase;
    uint32_t freeListHead;
    uint32_t freeListTail;
};

template<typename T>
SlotMap<T>::Slot
SlotMap<T>::Push(T value)
{
    // If free list empty:
    if (freeListHead == UINT32_MAX) {
        AddFreeListSLots();
    }

    data.push_back(std::move(value));
    erase.push_back(freeListHead);

    auto nextHead = indices[freeListHead].index;

    Slot dataSlot;
    dataSlot.index = data.size() - 1;
    dataSlot.generation = indices[freeListHead].generation + 1;
    indices[freeListHead] = dataSlot;

    Slot returnSlot;
    returnSlot.index = freeListHead;
    returnSlot.generation = dataSlot.generation;

    freeListHead = nextHead;

    return returnSlot;
}

template<typename T>
void
SlotMap<T>::Remove(Slot slot)
{
    if (!Valid(slot)) {
        ThrowError("Tried to remove out of date slot");
    }
    // Mark out of date
    ++indices[slot.index].generation;
    auto index = indices[slot.index].index;
    // Swap last into this slot
    // Requires erase table

    // Firstly swap this with the final data item
    if (index != data.size() - 1) {
        data[index] = std::move(data[data.size() - 1]);
        erase[index] = erase[data.size() - 1];
        indices[erase[index]].index = index;
    }

    // Then pop back to delete this item
    data.pop_back();
    erase.pop_back();

    // Add to free list
    indices[freeListTail].index = slot.index;
    indices[slot.index].index = UINT32_MAX;
    freeListTail = slot.index;
}

template<typename T>
T
SlotMap<T>::Get(Slot slot)
{
    if (!Valid(slot)) {
        ThrowError("Attempted to read an outdated slot");
    }
    return data[indices[slot.index].index];
}

template<typename T>
bool
SlotMap<T>::Valid(Slot slot)
{
    return indices[slot.index].generation == slot.generation;
}

template<typename T>
void
SlotMap<T>::AddFreeListSLots()
{
    // For our typical use case i.e. asset loading, it's not expected that the
    // list will need to change size much after startup
    // thus no geometric growth
    constexpr size_t RESERVE_SIZE = 32;

    indices.reserve(indices.size() + RESERVE_SIZE);

    uint32_t counter = indices.size();

    freeListHead = counter;

    for (size_t i = 0; i < RESERVE_SIZE; i++) {
        indices.push_back({ .index = ++counter });
    }

    // Mark last
    indices[counter - 1].index = UINT32_MAX;

    freeListTail = counter - 1;
}

}
