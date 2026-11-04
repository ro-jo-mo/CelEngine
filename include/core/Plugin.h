#pragma once

#include "PluginManager.h"

#include <concepts>

namespace Cel {
/**
 * @brief Required interface for plugins
 * A plugin is any type with a build method taking a PluginManager, used to
 * set up the plugin's resources and systems. Plugin configuration can be
 * stored as plain data members, e.g.
 *
 * struct MyPlugin
 * {
 *     bool enableFeature = true;
 *
 *     void build(PluginManager& manager);
 * };
 *
 * app.add_plugin(MyPlugin{ .enableFeature = false });
 */
template<typename T>
concept Plugin = requires(T& plugin, PluginManager& manager) {
    { plugin.build(manager) } -> std::same_as<void>;
};
}
