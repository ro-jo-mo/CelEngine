#pragma once
#include <typeindex>
#include <unordered_set>

class Locks {
public:
    template<typename T>
    void Register();

private:
    bool worldAccessed = false;
    std::unordered_set<std::type_index> write;
    std::unordered_set<std::type_index> read;
};

template<typename T>
void Locks::Register() {
    // Iterate over With<...> in queries
    // Special bool for world access
    if (std::is_const_v<T>) {
        read.insert(std::type_index(typeid(T)));
    } else {
    }
}
