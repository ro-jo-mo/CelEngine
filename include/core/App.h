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
        resourceManager.InsertResource<Time>();
        resourceManager.InsertResource<World>(componentsManager, entityManager);
        resourceManager.InsertResource<Running>();
    }

    template<ScheduleEnum... ScheduleEnums>
    App& Start(bool multithread = false);
    template<typename... Schedules>
        requires(IsSchedule<Schedules>::value && ...)
    App& Loop();
    template<ScheduleEnum... ScheduleEnums>
    App& End(bool multithread = false);

    /**
     * @brief Add a plugin to the game.
     * @tparam T Plugin type
     * @return A reference to this ecs, for chaining calls
     */
    template<typename T>
        requires std::derived_from<T, Plugin>
    App& AddPlugin();

  private:
    template<ScheduleEnum Enum>
    void ExecuteSchedule();

    template<typename Schedule>
        requires(IsSchedule<Schedule>::value)
    void ExecuteLoopedSchedule(Resource<Time>& time);

    void Flush();

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
App::ExecuteSchedule()
{
    bool found = false;
    const std::type_index id = typeid(Enum);

    for (auto& [key, executionGraph] : schedules) {
        if (key.schedule == id) {
            executionGraph.Execute();
            found = true;

            Flush();
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
App::ExecuteLoopedSchedule(Resource<Time>& time)
{
    using Enum = Schedule::ScheduleEnum;

    if constexpr (Schedule::IsFixed) {
        time->SwitchToFixed<Enum>();

        while (time->FixedUpdateRequired<Enum>()) {
            ExecuteSchedule<Enum>();

            time->FixedTick<Enum>();
        }
    } else {
        time->SwitchToDynamic();

        ExecuteSchedule<Enum>();
    }
}

template<ScheduleEnum... ScheduleEnums>
App&
App::Start(bool multithread)
{
    (void(ExecuteSchedule<ScheduleEnums>()), ...);

    resourceManager.GetResource<World>()->Flush();
    queryManager.UpdateQueries();

    return *this;
}

template<typename... Schedules>
    requires(IsSchedule<Schedules>::value && ...)
App&
App::Loop()
{
    auto& time = resourceManager.GetResource<Time>();
    auto& running = resourceManager.GetResource<Running>();

    (void(time->RegisterSchedule<Schedules>()), ...);

    running->isRunning = true;

    while (running->isRunning) {

        (void(ExecuteLoopedSchedule<Schedules>(time)), ...);

        Flush();
        time->Tick();
    }

    return *this;
}

template<ScheduleEnum... ScheduleEnums>
App&
App::End(bool multithread)
{
    (void(ExecuteSchedule<ScheduleEnums>()), ...);

    resourceManager.GetResource<World>()->Flush();
    queryManager.UpdateQueries();

    return *this;
}

template<typename T>
    requires std::derived_from<T, Plugin>
App&
App::AddPlugin()
{
    T().Build(Scheduler(schedules, systemAllocator), resourceManager);
    return *this;
}

inline void
App::Flush()
{
    resourceManager.GetResource<World>()->Flush();
    queryManager.UpdateQueries();
}

}
