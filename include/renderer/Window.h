#pragma once

#include "../core/Running.h"
#include "VulkanTypes.h"
#include "ecs/System.h"

#include <SDL3/SDL.h>
#include <vulkan/vulkan_core.h>

namespace Cel::Renderer {
struct Window
{
    Window();

    SDL_Window* window;
};

void
SetRenderExtent(Resource<RenderExtent>& renderExtent,
                Resource<DrawImage>& drawImage,
                Resource<Swapchain>& swapchain);
}