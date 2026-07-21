#include "ecs/ScheduleGraph.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <typeindex>
#include <unordered_set>

using namespace Cel;

void
ScheduleGraph::add_edge(void* from, void* to)
{
    adjacencyList[from].insert(to);
    requirements[to].insert(from);
}

void
ScheduleGraph::execute()
{
    std::unordered_set<void*> executed;

    for (const auto& [id, requirement] : requirements) {
        if (requirement.empty()) {
            execute_system(id, executed);
        }
    }
    assert(executed.size() == adjacencyList.size() &&
           "Cycles found in dependencies, not all systems executed");
}

void
ScheduleGraph::execute_system(void* id, std::unordered_set<void*>& executed)
{
    // if this node has no requirements, we can start executing it
    idToSystem[id]();
    executed.insert(id);

    // additionally execute nodes that require this
    recursively_execute(id, executed);
}

bool
ScheduleGraph::check_requirements(void* id, std::unordered_set<void*>& executed)
{
    return std::ranges::all_of(requirements[id], [&executed](auto& required) {
        return executed.contains(required);
    });
}

void
ScheduleGraph::recursively_execute(void* parentId,
                                  std::unordered_set<void*>& executed)
{
    for (auto& id : adjacencyList[parentId]) {
        if (check_requirements(id, executed)) {
            execute_system(id, executed);
        }
    }
}
