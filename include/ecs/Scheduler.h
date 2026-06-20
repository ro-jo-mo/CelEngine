#pragma once

#include "Schedule.h"
#include "ScheduleGraph.h"

#include <map>
#include <vector>

namespace Cel {

struct ScheduleKey
{
    std::type_index schedule;
    uint32_t index;
    auto operator<=>(const ScheduleKey&) const = default;
};
}

template<>
struct std::hash<Cel::ScheduleKey>
{
    size_t operator()(const Cel::ScheduleKey& key) const noexcept
    {
        const uint64_t packed = (key.schedule.hash_code() << 32) | key.index;
        return std::hash<uint64_t>{}(packed);
    }
};

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

    template<typename System>
    RelativeScheduler& After(System runsBefore);

    RelativeScheduler& Before(const RelativeScheduler& runsAfter);

    template<typename System>
    RelativeScheduler& Before(System runsAfter);

  private:
    ScheduleGraph& graph;
    std::vector<void*> exit;
    std::vector<void*> entrance;
};

template<typename System>
RelativeScheduler&
RelativeScheduler::After(System runsBefore)
{
    graph.AddNode(runsBefore);

    for (const auto& entry : this->entrance) {
        graph.AddEdge(reinterpret_cast<void*>(runsBefore), entry);
    }

    return *this;
}

template<typename System>
RelativeScheduler&
RelativeScheduler::Before(System runsAfter)
{
    graph.AddNode(runsAfter);

    for (const auto& exits : this->exit) {
        graph.AddEdge(exits, reinterpret_cast<void*>(runsAfter));
    }

    return *this;
}

/**
 * @brief A class for scheduling new systems
 */
class Scheduler
{
  public:
    explicit Scheduler(std::map<ScheduleKey, ScheduleGraph>& schedules,
                       SystemAllocator& systemAllocator)
        : schedules(schedules)
        , systemAllocator(systemAllocator)
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
    template<ScheduleEnum Schedule, typename System>
    RelativeScheduler AddSystem(Schedule schedule, System system);

    template<ScheduleEnum Schedule, typename... Systems>
    RelativeScheduler AddGroup(Schedule schedule, Systems... systems);

    template<ScheduleEnum Schedule, typename... Systems>
    RelativeScheduler AddChain(Schedule schedule, Systems... systems);

  private:
    template<ScheduleEnum Schedule>
    ScheduleGraph& GetGraph(Schedule schedule);

    std::map<ScheduleKey, ScheduleGraph>& schedules;
    SystemAllocator& systemAllocator;
};

template<ScheduleEnum Schedule>
ScheduleGraph&
Scheduler::GetGraph(Schedule schedule)
{
    return schedules
        .try_emplace({ typeid(Schedule), static_cast<uint32_t>(schedule) },
                     systemAllocator)
        .first->second;
}

template<ScheduleEnum Schedule, typename System>
RelativeScheduler
Scheduler::AddSystem(Schedule schedule, System system)
{
    auto& graph = GetGraph(schedule);

    graph.AddNode(system);
    return RelativeScheduler{ graph, reinterpret_cast<void*>(system) };
}

template<ScheduleEnum Schedule, typename... Systems>
RelativeScheduler
Scheduler::AddGroup(Schedule schedule, Systems... systems)
{
    auto& graph = GetGraph(schedule);

    (void(graph.AddNode(systems)), ...);

    return RelativeScheduler{ graph, { reinterpret_cast<void*>(systems)... } };
}

template<ScheduleEnum Schedule, typename... Systems>
RelativeScheduler
Scheduler::AddChain(Schedule schedule, Systems... systems)
{
    auto& graph = GetGraph(schedule);

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
                              reinterpret_cast<void*>(first),
                              reinterpret_cast<void*>(last) };
}

}
