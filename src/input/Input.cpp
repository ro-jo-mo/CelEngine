#include "input/Input.h"

#include "renderer/components/Window.h"

#include <SDL3/SDL_events.h>

bool
Cel::Input::Input::key_up(SDL_Scancode key) const
{
    return keyReleasedThisFrame[key];
}

bool
Cel::Input::Input::key_down(SDL_Scancode key) const
{
    return keyPressedThisFrame[key];
}

bool
Cel::Input::Input::key_held(SDL_Scancode key) const
{
    // Down now, and not freshly pressed this frame, i.e. it was down last frame
    return keyState[key] && !keyPressedThisFrame[key];
}

glm::vec2
Cel::Input::Input::mouse_scroll() const
{
    return mouseScroll;
}

glm::vec2
Cel::Input::Input::mouse_delta() const
{
    return mouseDelta;
}

bool
Cel::Input::Input::mouse_button_up(uint32_t button) const
{
    return !currentMouseButtonState[button] && previousMouseButtonState[button];
}

bool
Cel::Input::Input::mouse_button_down(uint32_t button) const
{
    return currentMouseButtonState[button] && !previousMouseButtonState[button];
}

bool
Cel::Input::Input::mouse_button_held(uint32_t button) const
{
    return currentMouseButtonState[button] && previousMouseButtonState[button];
}

void
Cel::Input::Input::tick_input()
{
    // Reset key up down
    keyPressedThisFrame.fill(false);
    keyReleasedThisFrame.fill(false);

    // copy last frames input buffer into array
    previousMouseButtonState = currentMouseButtonState;

    mouseDelta = glm::vec2(0.0f);
    mouseScroll = glm::vec2(0.0f);

    // Clear last frames events
    events.clear();
}

void
Cel::Input::process_input_events(Resource<Input>& input,
                                 Resource<Running>& isRunning,
                                 Resource<Renderer::Window>& window)
{
    input->tick_input();

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        input->events.push_back(event);

        switch (event.type) {
            case SDL_EVENT_QUIT:
                isRunning->isRunning = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (!event.key.repeat) {
                    input->keyPressedThisFrame[event.key.scancode] = true;
                }
                input->keyState[event.key.scancode] = true;
                break;
            case SDL_EVENT_KEY_UP:
                input->keyReleasedThisFrame[event.key.scancode] = true;
                input->keyState[event.key.scancode] = false;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                input->mouseScroll += glm::vec2{ event.wheel.x, event.wheel.y };
                break;
            case SDL_EVENT_MOUSE_MOTION:
                input->mouseDelta +=
                    glm::vec2{ event.motion.xrel, event.motion.yrel };
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                input->currentMouseButtonState[event.button.button] = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                input->currentMouseButtonState[event.button.button] = false;
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                window->resized = true;
                break;
            case SDL_EVENT_WINDOW_MINIMIZED:
                window->minimized = true;
                break;
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_MAXIMIZED:
                window->minimized = false;
                // The surface may have changed size while minimised
                window->resized = true;
                break;
            default:
                // Unimplemented events
                break;
        }
    }
}
