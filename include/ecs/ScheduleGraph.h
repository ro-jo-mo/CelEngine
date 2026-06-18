#pragma once

#include "System.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace Cel {
/**
 * @brief A directed acyclic graph representing a partial ordering of systems
 */
class ScheduleGraph
{
  public:
    explicit ScheduleGraph(SystemAllocator& system_allocator)
        : systemAllocator(system_allocator) {};

    /**
     * @brief Add a new system to the graph, with no edges
     * @tparam System System type
     */
    template<typename System>
    void AddNode(System system);

    /**
     * @brief  Add an edge marking "from" as a dependency for "to"
     * @param from Function pointer
     * @param to Function pointer
     */
    void AddEdge(void* from, void* to);

    /**
     * @brief Serial execution of this DAG
     */
    void Execute();

  private:
    /**
     * @brief Execute an individual system, and conditionally run its dependents
     * @param id System id to run
     * @param executed A set of already executed systems
     */
    void ExecuteSystem(void* id, std::unordered_set<void*>& executed);

    /**
     * @brief Returns true if all the dependencies of this system have been run
     * @param id System to check
     * @param executed A set of already executed systems
     * @return True if system is ready to run, false otherwise
     */
    bool CheckRequirements(void* id, std::unordered_set<void*>& executed);

    /**
     * @brief Execute all child systems ready to go
     * @param id Parent system
     * @param executed A set of already executed systems
     */
    void RecursivelyExecute(void* id, std::unordered_set<void*>& executed);

    std::unordered_map<void*, std::unordered_set<void*>> adjacencyList{};
    std::unordered_map<void*, std::unordered_set<void*>> requirements{};
    std::unordered_map<void*, std::function<void()>> idToSystem;

    SystemAllocator& systemAllocator;
};

template<typename System>
void
ScheduleGraph::AddNode(System system)
{
    // Firstly check if system already exists
    if (idToSystem.contains(reinterpret_cast<void*>(system))) {
        return;
    }

    idToSystem[reinterpret_cast<void*>(system)] =
        systemAllocator.AllocateSystem(system);
    requirements[reinterpret_cast<void*>(system)];
    adjacencyList[reinterpret_cast<void*>(system)];
}

}
