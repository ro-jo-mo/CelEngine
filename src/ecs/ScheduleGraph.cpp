#include "ecs/ScheduleGraph.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <typeindex>
#include <unordered_set>

using namespace Cel;

void
ScheduleGraph::AddEdge(void* from, void* to)
{
    adjacencyList[from].insert(to);
    requirements[to].insert(from);
}

void
ScheduleGraph::Execute()
{
    std::unordered_set<void*> executed;

    for (const auto& [id, requirement] : requirements) {
        if (requirement.empty()) {
            ExecuteSystem(id, executed);
        }
    }
    assert(executed.size() == adjacencyList.size() &&
           "Cycles found in dependencies, not all systems executed");
}

void
ScheduleGraph::ExecuteSystem(void* id, std::unordered_set<void*>& executed)
{
    // if this node has no requirements, we can start executing it
    idToSystem[id]();
    executed.insert(id);

    // additionally execute nodes that require this
    RecursivelyExecute(id, executed);
}

bool
ScheduleGraph::CheckRequirements(void* id, std::unordered_set<void*>& executed)
{
    return std::ranges::all_of(requirements[id], [&executed](auto& required) {
        return executed.contains(required);
    });
}

void
ScheduleGraph::RecursivelyExecute(void* parentId,
                                  std::unordered_set<void*>& executed)
{
    for (auto& id : adjacencyList[parentId]) {
        if (CheckRequirements(id, executed)) {
            ExecuteSystem(id, executed);
        }
    }
}
