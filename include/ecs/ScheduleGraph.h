#pragma once

#include "System.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace Cel {
  class ScheduleGraph {
  public:
    explicit ScheduleGraph(SystemAllocator &system_allocator)
      : systemAllocator(system_allocator) {
    };

    template<typename T>
    void AddNode();

    template<typename From, typename To>
    void AddEdge();

    void Execute();

  private:
    void ExecuteSystem(std::type_index id,
                       std::unordered_set<std::type_index> &executed);

    bool CheckRequirements(std::type_index id,
                           std::unordered_set<std::type_index> &executed);

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


