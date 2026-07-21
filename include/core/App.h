#pragma once

#include "Plugin.h"
#include "Running.h"
#include "core/Time.h"
#include "core/World.h"
#include "ecs/ComponentsManager.h"
#include "ecs/EntityManager.h"
#include "ecs/QueryManager.h"
#include "ecs/ResourceManager.h"
#include "ecs/ScheduleGraph.h"
#include "ecs/Scheduler.h"

#include <concepts>
#include <map>

namespace Cel {
/**
 * @brief The primary entry point for the ecs API
 * Handles plugins, entity & resource & component storage, system scheduling.
 */
class App
{
  public:
    App()
        : componentsManager(ComponentsManager())
        , queryManager(componentsManager)
        , systemAllocator(resourceManager, queryManager)
    {
        resourceManager.insert_resource<Time>();
        resourceManager.insert_resource<World>(componentsManager, entityManager);
        resourceManager.insert_resource<Running>();
    }

    template<ScheduleEnum... ScheduleEnums>
    App& start(bool multithread = false);
    template<typename... Schedules>
        requires(IsSchedule<Schedules>::value && ...)
    App& loop();
    template<ScheduleEnum... ScheduleEnums>
    App& end(bool multithread = false);

    /**
     * @brief Add a plugin to the game.
     * @tparam T Plugin type
     * @return A reference to this ecs, for chaining calls
     */
    template<typename T>
        requires std::derived_from<T, Plugin>
    App& add_plugin();

  private:
    template<ScheduleEnum Enum>
    void execute_schedule();

    template<typename Schedule>
        requires(IsSchedule<Schedule>::value)
    void execute_looped_schedule(Resource<Time>& time);

    void flush();

    ComponentsManager componentsManager;
    EntityManager entityManager;
    ResourceManager resourceManager;
    QueryManager queryManager;
    SystemAllocator systemAllocator;

    // Indexed by [{typeid(Schedule),schedule}]
    std::map<ScheduleKey, ScheduleGraph> schedules;
};

template<ScheduleEnum Enum>
void
App::execute_schedule()
{
    bool found = false;
    const std::type_index id = typeid(Enum);

    for (auto& [key, executionGraph] : schedules) {
        if (key.schedule == id) {
            executionGraph.execute();
            found = true;

            flush();
            continue;
        }
        if (found == true) {
            break;
        }
    }
}

template<typename Schedule>
    requires(IsSchedule<Schedule>::value)
void
App::execute_looped_schedule(Resource<Time>& time)
{
    using Enum = Schedule::ScheduleEnum;

    if constexpr (Schedule::IsFixed) {
        time->switch_to_fixed<Enum>();

        while (time->is_fixed_update_required<Enum>()) {
            execute_schedule<Enum>();

            time->fixed_tick<Enum>();
        }
    } else {
        time->switch_to_dynamic();

        execute_schedule<Enum>();
    }
}

template<ScheduleEnum... ScheduleEnums>
App&
App::start(bool multithread)
{
    (void(execute_schedule<ScheduleEnums>()), ...);

    resourceManager.GetResource<World>()->flush();
    queryManager.update_queries();

    return *this;
}

template<typename... Schedules>
    requires(IsSchedule<Schedules>::value && ...)
App&
App::loop()
{
    auto& time = resourceManager.GetResource<Time>();
    auto& running = resourceManager.GetResource<Running>();

    (void(time->register_schedule<Schedules>()), ...);

    running->isRunning = true;

    while (running->isRunning) {

        (void(execute_looped_schedule<Schedules>(time)), ...);

        flush();
        time->tick();
    }

    return *this;
}

template<ScheduleEnum... ScheduleEnums>
App&
App::end(bool multithread)
{
    (void(execute_schedule<ScheduleEnums>()), ...);

    resourceManager.GetResource<World>()->flush();
    queryManager.update_queries();

    return *this;
}

template<typename T>
    requires std::derived_from<T, Plugin>
App&
App::add_plugin()
{
    T().build(Scheduler(schedules, systemAllocator), resourceManager);
    return *this;
}

inline void
App::flush()
{
    resourceManager.GetResource<World>()->flush();
    queryManager.update_queries();
}

}
