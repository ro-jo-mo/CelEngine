#include "renderer/Window.h"
#include <SDL3/SDL_vulkan.h>

Cel::Renderer::Window::Window()
{
    constexpr SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    window = SDL_CreateWindow("CelEngine", extent.width, extent.height, flags);
    // Add cleanup
}

void
Cel::Renderer::WindowSystem::Run(Resource<Window>& window,
                                 Resource<Running>& isRunning)
{
}
