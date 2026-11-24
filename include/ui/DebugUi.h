#pragma once

#include "core/Plugin.h"
#include "ecs/Resource.h"

#include <SDL3/SDL_scancode.h>

namespace Cel {
namespace Input {
class Input;
}
namespace Renderer {
struct VulkanContext;
struct Window;
struct RenderExtent;
}
}
namespace Cel::Ui {

// Dummy resource used to synchronise access to ImGui, as ImGui stores a global
// state and is not thread safe :(
struct DebugUi
{
    bool visible;
    SDL_Scancode toggleKey;
};

struct DebugUiPlugin
{
    SDL_Scancode toggleKey = SDL_SCANCODE_GRAVE;

    void build(PluginManager& manager);
};

void
init_imgui(Resource<Renderer::VulkanContext>& context,
           Resource<Renderer::Window>& window);

// Starts the ImGui frame. Runs after input is processed
void
debug_ui_begin(Resource<DebugUi>& ui, Resource<Input::Input>& input);

// Ends the ImGui frame & uploads any changed textures (font atlas). Runs after
// the render extent is set for this frame
void
debug_ui_end(Resource<Renderer::RenderExtent>& extent);

void
teardown_imgui(Resource<Renderer::VulkanContext>& context);

}
