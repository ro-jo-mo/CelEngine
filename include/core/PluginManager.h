#pragma once

#include "ecs/ResourceManager.h"
#include "ecs/SystemScheduler.h"

#include <utility>

namespace Cel {
/**
 * An object passed to plugins for registration. Allows access to inserting
 * resources and scheduling systems
 */
class PluginManager
{
  public:
    PluginManager(SystemScheduler scheduler, ResourceManager& resourceManager)
        : scheduler(scheduler)
        , resourceManager(resourceManager)
    {
    }

    /**
     * @brief Initialise a new resource
     * @tparam T Resource type
     * @tparam Args Argument types
     * @param args Arguments to initialise resource with
     */
    template<typename T, typename... Args>
    void insert_resource(Args&&... args);

    template<typename T>
    void insert_resource(T resource);

    template<typename T>
    void insert_resource_as_null();

    template<typename T, typename... Args>
    void insert_parallel_resource(Args&&... args);

    template<typename T>
    void insert_parallel_resource(T resource);

    template<typename T>
    void insert_parallel_resource_as_null();

    // Systems

    /**
     * @brief Add a new system to a schedule
     * @tparam Schedule Schedule enum type
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

    /**
     * @brief Add a group of unordered systems to a schedule
     */
    template<ScheduleEnum Schedule, typename... Systems>
    Common::RelativeScheduler<void*, ScheduleGraph> add_group(
        Schedule schedule,
        Systems... systems);

    /**
     * @brief Add a chain of systems to a schedule, run in the order given
     */
    template<ScheduleEnum Schedule, typename... Systems>
    Common::RelativeScheduler<void*, ScheduleGraph> add_chain(
        Schedule schedule,
        Systems... systems);

  private:
    SystemScheduler scheduler;
    ResourceManager& resourceManager;
};

template<typename T, typename... Args>
void
PluginManager::insert_resource(Args&&... args)
{
    resourceManager.insert_resource<T>(std::forward<Args>(args)...);
}

template<typename T>
void
PluginManager::insert_resource(T resource)
{
    resourceManager.insert_resource<T>(std::move(resource));
}

template<typename T>
void
PluginManager::insert_resource_as_null()
{
    resourceManager.insert_resource_as_null<T>();
}

template<typename T, typename... Args>
void
PluginManager::insert_parallel_resource(Args&&... args)
{
    resourceManager.insert_parallel_resource<T>(std::forward<Args>(args)...);
}

template<typename T>
void
PluginManager::insert_parallel_resource(T resource)
{
    resourceManager.insert_parallel_resource<T>(std::move(resource));
}

template<typename T>
void
PluginManager::insert_parallel_resource_as_null()
{
    resourceManager.insert_parallel_resource_as_null<T>();
}

template<ScheduleEnum Schedule, typename System>
Common::RelativeScheduler<void*, ScheduleGraph>
PluginManager::add_system(Schedule schedule, System system)
{
    return scheduler.add_system(schedule, system);
}

template<ScheduleEnum Schedule, typename... Systems>
Common::RelativeScheduler<void*, ScheduleGraph>
PluginManager::add_group(Schedule schedule, Systems... systems)
{
    return scheduler.add_group(schedule, systems...);
}

template<ScheduleEnum Schedule, typename... Systems>
Common::RelativeScheduler<void*, ScheduleGraph>
PluginManager::add_chain(Schedule schedule, Systems... systems)
{
    return scheduler.add_chain(schedule, systems...);
}

}
