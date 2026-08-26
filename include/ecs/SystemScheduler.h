#pragma once

#include "Schedule.h"
#include "ScheduleGraph.h"
#include "common/Scheduler.h"

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

/**
 * A class for scheduling new systems
 * LIMITATIONS
 * While a system can run in multiple schedules, for example hierarchy
 * propagation runs at the end of start and at the end of the main update, a
 * system cannot run twice in the same schedule, e.g. in PreUpdate &
 * PostUpdate
 */
class SystemScheduler : Common::Detail::BaseScheduler<void*>
{
  public:
    explicit SystemScheduler(std::map<ScheduleKey, ScheduleGraph>& schedules,
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
    Common::RelativeScheduler<void*, ScheduleGraph> add_system(
        Schedule schedule,
        System system);

    template<ScheduleEnum Schedule, typename... Systems>
    Common::RelativeScheduler<void*, ScheduleGraph> add_group(
        Schedule schedule,
        Systems... systems);

    template<ScheduleEnum Schedule, typename... Systems>
    Common::RelativeScheduler<void*, ScheduleGraph> add_chain(
        Schedule schedule,
        Systems... systems);

  private:
    template<ScheduleEnum Schedule>
    ScheduleGraph& get_graph(Schedule schedule);

    std::map<ScheduleKey, ScheduleGraph>& schedules;
    SystemAllocator& systemAllocator;
};

template<ScheduleEnum Schedule>
ScheduleGraph&
SystemScheduler::get_graph(Schedule schedule)
{
    return schedules
        .try_emplace({ typeid(Schedule), static_cast<uint32_t>(schedule) },
                     systemAllocator)
        .first->second;
}

template<ScheduleEnum Schedule, typename System>
Common::RelativeScheduler<void*, ScheduleGraph>
SystemScheduler::add_system(Schedule schedule, System system)
{
    auto& graph = get_graph(schedule);
    graph.add_node(system);
    return add_system_impl(graph, reinterpret_cast<void*>(system));
}

template<ScheduleEnum Schedule, typename... Systems>
Common::RelativeScheduler<void*, ScheduleGraph>
SystemScheduler::add_group(Schedule schedule, Systems... systems)
{
    auto& graph = get_graph(schedule);
    (void(graph.add_node(systems)), ...);
    return add_group_impl(graph, reinterpret_cast<void*>(systems)...);
}

template<ScheduleEnum Schedule, typename... Systems>
Common::RelativeScheduler<void*, ScheduleGraph>
SystemScheduler::add_chain(Schedule schedule, Systems... systems)
{
    auto& graph = get_graph(schedule);
    (void(graph.add_node(systems)), ...);
    return add_chain_impl(graph, reinterpret_cast<void*>(systems)...);
}

}
