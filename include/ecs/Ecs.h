#pragma once

#include "ComponentsManager.h"
#include "EntityManager.h"
#include "ResourceManager.h"
#include "QueryManager.h"
#include <concepts>
#include "Plugin.h"
#include "ScheduleGraph.h"
#include "Time.h"
#include "World.h"
#include "Schedule.h"
#include "Scheduler.h"

namespace Cel {
  /**
   * @brief The primary entry point for the ecs API
   * Handles plugins, entity & resource & component storage, system scheduling.
   */
  class Ecs {
  public:
    Ecs() : componentsManager(ComponentsManager()),
            queryManager(componentsManager),
            systemAllocator(resourceManager, queryManager) {
      resourceManager.InsertResource<Time>(1.0f / 60.0f);
      resourceManager.InsertResource<World>(componentsManager, entityManager);
      for (std::size_t i = 0; i < Schedule::SIZE; ++i) {
        schedules.emplace_back(systemAllocator);
      }
    }

    /**
     * @brief Runs the main game loop
     */
    void Run();

    /**
     * @brief Add a plugin to the game.
     * @tparam T Plugin type
     * @return A reference to this ecs, for chaining calls
     */
    template<typename T>
      requires std::derived_from<T, Plugin>
    Ecs &AddPlugin();

  private:
    ComponentsManager componentsManager;
    EntityManager entityManager;
    ResourceManager resourceManager;
    QueryManager queryManager;
    SystemAllocator systemAllocator;
    std::vector<ScheduleGraph> schedules;
  };

  template<typename T> requires std::derived_from<T, Plugin>
  Ecs &Ecs::AddPlugin() {
    T().Build(Scheduler(schedules), resourceManager);
    return *this;
  }
}
