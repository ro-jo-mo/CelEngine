#pragma once

#include "System.h"
#include "SystemManager.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace Cel {
  template<typename T>
  concept IsASystem = std::derived_from<T, System> && std::constructible_from<T, SystemManager &>;

  class ScheduleGraph {
  public:
    explicit ScheduleGraph(SystemManager &systemManager)
      : manager(systemManager) {
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
    std::unordered_map<std::type_index, std::unique_ptr<System> > idToSystem{};
    SystemManager &manager;
  };

  template<typename T>
  void
  ScheduleGraph::AddNode() {
    idToSystem[typeid(T)] = std::make_unique<T>();
  }

  template<typename From, typename To>
  void
  ScheduleGraph::AddEdge() {
    adjacencyList[typeid(From)].insert(typeid(To));
    requirements[typeid(To)].insert(typeid(From));
  }
}


