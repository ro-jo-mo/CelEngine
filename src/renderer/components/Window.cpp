#include "../../../include/renderer/components/Window.h"
#include <SDL3/SDL_vulkan.h>

using namespace Cel;

Renderer::Window::Window()
{
    SDL_Init(SDL_INIT_VIDEO);

    constexpr SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;

    window = SDL_CreateWindow(
        "CelEngine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, flags);

    if (window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }
}

VkExtent2D
Renderer::Window::get_pixel_extent() const
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return { .width = static_cast<uint32_t>(width),
             .height = static_cast<uint32_t>(height) };
}

void
Renderer::set_render_extent(Resource<RenderExtent>& renderExtent,
                            Resource<Swapchain>& swapchain)
{
    renderExtent->extent.height =
        swapchain->extent.height * renderExtent->renderScale;
    renderExtent->extent.width =
        swapchain->extent.width * renderExtent->renderScale;
}
