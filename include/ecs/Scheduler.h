#pragma once

#include "ScheduleGraph.h"
#include <vector>

namespace Cel {

class RelativeScheduler
{
  public:
    RelativeScheduler(ScheduleGraph& graph, void* system)
        : graph(graph)
        , exit{ system }
        , entrance{ system }
    {
    }

    RelativeScheduler(ScheduleGraph& graph, void* start, void* end)
        : graph(graph)
        , exit{ end }
        , entrance{ start }
    {
    }

    RelativeScheduler(ScheduleGraph& graph, std::vector<void*>&& systems)
        : graph(graph)
        , exit(systems)
        , entrance(systems)
    {
    }

    RelativeScheduler& After(const RelativeScheduler& runsBefore);

    RelativeScheduler& After(const void* runsBefore);

    RelativeScheduler& Before(const RelativeScheduler& runsAfter);

    RelativeScheduler& Before(const void* runsAfter);

  private:
    ScheduleGraph& graph;
    std::vector<void*> exit;
    std::vector<void*> entrance;
};

/**
 * @brief A class for scheduling new systems
 */
class Scheduler
{
  public:
    explicit Scheduler(auto& scdl)
        : schedules(scdl)
    {
    }

    /**
     * @brief Add a new system to this schedule
     * @tparam System System function type
     * @param schedule Schedule to add this system to i.e (update, fixed update,
     * ...)
     * @param system System to add
     * @return A scheduling object for ordering this system relative to others
     */
    template<typename Schedule, typename System>
    RelativeScheduler AddSystem(Schedule schedule, System system);

    template<typename Schedule, typename... Systems>
    RelativeScheduler AddGroup(Schedule schedule, Systems... systems);

    template<typename Schedule, typename... Systems>
    RelativeScheduler AddChain(Schedule schedule, Systems... systems);

  private:
    std::vector<ScheduleGraph>& schedules;
};

template<typename Schedule, typename System>
RelativeScheduler
Scheduler::AddSystem(Schedule schedule, System system)
{
    auto& graph = schedules[schedule];
    graph.AddNode(system);
    return RelativeScheduler{ graph, reinterpret_cast<void*>(&system) };
}

template<typename Schedule, typename... Systems>
RelativeScheduler
Scheduler::AddGroup(Schedule schedule, Systems... systems)
{
    auto& graph = schedules[schedule];
    (void(graph.AddNode(systems)), ...);

    return RelativeScheduler{ graph, { reinterpret_cast<void*>(&systems)... } };
}

template<typename Schedule, typename... Systems>
RelativeScheduler
Scheduler::AddChain(Schedule schedule, Systems... systems)
{
    auto& graph = schedules[schedule];
    (void(graph.AddNode(systems)), ...);

    auto tuple = std::make_tuple(systems...);
    constexpr size_t SIZE = sizeof...(Systems);

    [&]<size_t... Index>(std::index_sequence<Index...>) {
        (void(
             graph.AddEdge(std::get<Index>(tuple), std::get<Index + 1>(tuple))),
         ...);
    }(std::make_index_sequence<SIZE - 1>{});

    auto first = std::get<0>(tuple);
    auto last = std::get<SIZE - 1>(tuple);

    return RelativeScheduler{ graph,
                              reinterpret_cast<void*>(&first),
                              reinterpret_cast<void*>(&last) };
}

}
