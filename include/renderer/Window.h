#pragma once

#include "../core/Running.h"
#include "RenderTypes.h"
#include "ecs/System.h"

#include <SDL3/SDL.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {

struct Window
{
    static constexpr int DEFAULT_WINDOW_WIDTH = 1600;
    static constexpr int DEFAULT_WINDOW_HEIGHT = 900;

    Window();

    // The current drawable size of the window in pixels. This may differ from
    // the window size on high DPI displays. Zero while minimised on some
    // platforms
    [[nodiscard]] VkExtent2D get_pixel_extent() const;

    SDL_Window* window;

    bool resized = false;
    bool minimized = false;
};

void
set_render_extent(Resource<RenderExtent>& renderExtent,
                  Resource<Swapchain>& swapchain);
}
