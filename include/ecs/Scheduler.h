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

    RelativeScheduler& after(const RelativeScheduler& runsBefore);

    template<typename System>
    RelativeScheduler& after(System runsBefore);

    RelativeScheduler& before(const RelativeScheduler& runsAfter);

    template<typename System>
    RelativeScheduler& before(System runsAfter);

  private:
    ScheduleGraph& graph;
    std::vector<void*> exit;
    std::vector<void*> entrance;
};

template<typename System>
RelativeScheduler&
RelativeScheduler::after(System runsBefore)
{
    graph.add_node(runsBefore);

    for (const auto& entry : this->entrance) {
        graph.add_edge(reinterpret_cast<void*>(runsBefore), entry);
    }

    return *this;
}

template<typename System>
RelativeScheduler&
RelativeScheduler::before(System runsAfter)
{
    graph.add_node(runsAfter);

    for (const auto& exits : this->exit) {
        graph.add_edge(exits, reinterpret_cast<void*>(runsAfter));
    }

    return *this;
}

/**
 * @brief A class for scheduling new systems
 * LIMITATIONS
 * While a system can run in multiple schedules, for example hierarchy
 * propagation runs at the end of start and at the end of the main update, a
 * system cannot run twice in the same schedule
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
    RelativeScheduler add_system(Schedule schedule, System system);

    template<ScheduleEnum Schedule, typename... Systems>
    RelativeScheduler add_group(Schedule schedule, Systems... systems);

    template<ScheduleEnum Schedule, typename... Systems>
    RelativeScheduler add_chain(Schedule schedule, Systems... systems);

  private:
    template<ScheduleEnum Schedule>
    ScheduleGraph& get_graph(Schedule schedule);

    std::map<ScheduleKey, ScheduleGraph>& schedules;
    SystemAllocator& systemAllocator;
};

template<ScheduleEnum Schedule>
ScheduleGraph&
Scheduler::get_graph(Schedule schedule)
{
    return schedules
        .try_emplace({ typeid(Schedule), static_cast<uint32_t>(schedule) },
                     systemAllocator)
        .first->second;
}

template<ScheduleEnum Schedule, typename System>
RelativeScheduler
Scheduler::add_system(Schedule schedule, System system)
{
    auto& graph = get_graph(schedule);

    graph.add_node(system);
    return RelativeScheduler{ graph, reinterpret_cast<void*>(system) };
}

template<ScheduleEnum Schedule, typename... Systems>
RelativeScheduler
Scheduler::add_group(Schedule schedule, Systems... systems)
{
    auto& graph = get_graph(schedule);

    (void(graph.add_node(systems)), ...);

    return RelativeScheduler{ graph, { reinterpret_cast<void*>(systems)... } };
}

template<ScheduleEnum Schedule, typename... Systems>
RelativeScheduler
Scheduler::add_chain(Schedule schedule, Systems... systems)
{
    auto& graph = get_graph(schedule);

    (void(graph.add_node(systems)), ...);

    auto tuple = std::make_tuple(systems...);
    constexpr size_t SIZE = sizeof...(Systems);

    [&]<size_t... Index>(std::index_sequence<Index...>) {
        (void(graph.add_edge(
             reinterpret_cast<void*>(std::get<Index>(tuple)),
             reinterpret_cast<void*>(std::get<Index + 1>(tuple)))),
         ...);
    }(std::make_index_sequence<SIZE - 1>{});

    auto first = std::get<0>(tuple);
    auto last = std::get<SIZE - 1>(tuple);

    return RelativeScheduler{ graph,
                              reinterpret_cast<void*>(first),
                              reinterpret_cast<void*>(last) };
}
}
