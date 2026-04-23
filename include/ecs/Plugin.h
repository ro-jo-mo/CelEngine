#pragma once

#include "ResourceManager.h"
#include "Scheduler.h"

namespace Cel {
    /**
     * @brief Required interface for plugins
     * Includes methods for the setup of plugin resources and systems
     */
    class Plugin {
    public:
        /**
         * @brief Adds the plugin to the ECS.
         * @param scheduler Use to schedule game systems
         * @param resourceManager Use to initialise resources
         */
        virtual void Build(Scheduler scheduler, ResourceManager &resourceManager) = 0;

        virtual ~Plugin() = default;
    };
}
