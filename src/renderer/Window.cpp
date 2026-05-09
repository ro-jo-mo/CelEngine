#include "renderer/Window.h"
#include <SDL3/SDL_vulkan.h>

Cel::Renderer::Window::Window()
{
    SDL_Init(SDL_INIT_VIDEO);

    constexpr SDL_WindowFlags flags = SDL_WINDOW_VULKAN;

    window = SDL_CreateWindow("CelEngine", extent.width, extent.height, flags);

    if (window == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }
}

void
Cel::Renderer::WindowSystem::Run(Resource<Window>& window,
                                 Resource<Running>& isRunning)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            isRunning->isRunning = false;
            break;
        }
    }
}
