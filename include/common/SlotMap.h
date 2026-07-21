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
    SlotMap() { add_free_list_slots(); }

    // Index points to the data array
    // Generation ensures a slot is still valid
    struct Slot
    {
        uint32_t index;
        uint32_t generation = 0;
    };

    Slot push(T value);
    void remove(Slot slot);
    T get(Slot slot);
    bool valid(Slot slot);

  private:
    void add_free_list_slots();

    std::vector<Slot> indices;
    std::vector<T> data;
    std::vector<uint32_t> erase;
    uint32_t freeListHead;
    uint32_t freeListTail;
};

template<typename T>
SlotMap<T>::Slot
SlotMap<T>::push(T value)
{
    // If free list empty:
    if (freeListHead == UINT32_MAX) {
        add_free_list_slots();
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
SlotMap<T>::remove(Slot slot)
{
    if (!valid(slot)) {
        throw_error("Tried to remove out of date slot");
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
SlotMap<T>::get(Slot slot)
{
    if (!valid(slot)) {
        throw_error("Attempted to read an outdated slot");
    }
    return data[indices[slot.index].index];
}

template<typename T>
bool
SlotMap<T>::valid(Slot slot)
{
    return indices[slot.index].generation == slot.generation;
}

template<typename T>
void
SlotMap<T>::add_free_list_slots()
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
