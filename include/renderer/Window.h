#pragma once

#include "ecs/Running.h"
#include "ecs/System.h"

#include <SDL3/SDL.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {
struct Window
{
    Window();

    SDL_Window* window;
    VkExtent2D extent{ .width = 1600, .height = 900 };
};

class WindowSystem final : public System<Resource<Window>, Resource<Running>>
{
  public:
    void Run(Resource<Window>& window, Resource<Running>& isRunning) override;
};
}