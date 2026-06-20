#include "renderer/Window.h"
#include <SDL3/SDL_vulkan.h>

using namespace Cel;

Renderer::Window::Window()
{
    SDL_Init(SDL_INIT_VIDEO);

    constexpr SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    window = SDL_CreateWindow("CelEngine", 1600, 900, flags);

    if (window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }
}

void
Renderer::WindowSystem(Resource<Window>& window, Resource<Running>& isRunning)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isRunning->isRunning = false;
            break;
        }
    }
}

void
Renderer::SetRenderExtent(Resource<RenderExtent>& renderExtent,
                          Resource<DrawImage>& drawImage,
                          Resource<Swapchain>& swapchain)
{
    renderExtent->extent.height =
        std::min(swapchain->extent.height, drawImage->imageExtent.height) *
        renderExtent->renderScale;
    renderExtent->extent.width =
        std::min(swapchain->extent.width, drawImage->imageExtent.width) *
        renderExtent->renderScale;
}
