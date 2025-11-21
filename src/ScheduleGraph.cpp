#include "ecs/ScheduleGraph.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <typeindex>
#include <unordered_set>

using namespace Cel;

void
ScheduleGraph::Execute() {
  std::unordered_set<std::type_index> executed;

  for (auto &[id, requirement]: requirements) {
    if (requirement.empty()) {
      ExecuteSystem(id, executed);
    }
  }
  assert(executed.size() == adjacencyList.size() &&
    "Cycles found in dependencies, not all systems executed");
}

void
ScheduleGraph::ExecuteSystem(const std::type_index id,
                             std::unordered_set<std::type_index> &executed) {
  // if this node has no requirements, we can start executing it
  idToSystem[id]->Execute();
  executed.insert(id);

  // additionally execute nodes that require this
  RecursivelyExecute(id, executed);
}

bool
ScheduleGraph::CheckRequirements(const std::type_index id,
                                 std::unordered_set<std::type_index> &executed) {
  return std::ranges::all_of(requirements[id],
                             [&executed](auto &required) {
                               return executed.contains(required);
                             });
}

void
ScheduleGraph::RecursivelyExecute(std::type_index parentId,
                                  std::unordered_set<std::type_index> &executed) {
  for (auto &id: adjacencyList[parentId]) {
    if (CheckRequirements(id, executed)) {
      ExecuteSystem(id, executed);
    }
  }
}
