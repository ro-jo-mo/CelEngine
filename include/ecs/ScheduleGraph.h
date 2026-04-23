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
  class ScheduleGraph {
  public:
    explicit ScheduleGraph(SystemAllocator &system_allocator)
      : systemAllocator(system_allocator) {
    };

    /**
     * @brief Add a new system to the graph, with no edges
     * @tparam T System type
     */
    template<typename T>
    void AddNode();

    /**
     * @brief Introduce a new edge between to systems (From -> To)
     * @tparam From System type
     * @tparam To System type
     */
    template<typename From, typename To>
    void AddEdge();

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
    void ExecuteSystem(std::type_index id,
                       std::unordered_set<std::type_index> &executed);

    /**
     * @brief Returns true if all the dependencies of this system have been run
     * @param id System to check
     * @param executed A set of already executed systems
     * @return True if system is ready to run, false otherwise
     */
    bool CheckRequirements(std::type_index id,
                           std::unordered_set<std::type_index> &executed);

    /**
     * @brief Execute all child systems ready to go
     * @param id Parent system
     * @param executed A set of already executed systems
     */
    void RecursivelyExecute(std::type_index id,
                            std::unordered_set<std::type_index> &executed);

    std::unordered_map<std::type_index, std::unordered_set<std::type_index> >
    adjacencyList{};
    std::unordered_map<std::type_index, std::unordered_set<std::type_index> >
    requirements{};
    std::unordered_map<std::type_index, std::function<void()> > idToSystem{};
    SystemAllocator &systemAllocator;
  };

  template<typename T>
  void ScheduleGraph::AddNode() {
    auto system = std::make_shared<T>();
    idToSystem[typeid(T)] = T::Register(system, systemAllocator);
    // Use registered items from sys allocator for dependency graphs
    requirements[typeid(T)];
    adjacencyList[typeid(T)];
  }

  template<typename From, typename To>
  void ScheduleGraph::AddEdge() {
    adjacencyList[typeid(From)].insert(typeid(To));
    requirements[typeid(To)].insert(typeid(From));
  }
}


