#include "input/Input.h"

#include <SDL3/SDL_events.h>
bool
Cel::Input::Input::KeyUp(SDL_Scancode key) const
{
    return !currentKeyState[key] && previousKeyState[key];
}

bool
Cel::Input::Input::KeyDown(SDL_Scancode key) const
{
    return currentKeyState[key] && !previousKeyState[key];
}

bool
Cel::Input::Input::KeyHeld(SDL_Scancode key) const
{
    return currentKeyState[key] && previousKeyState[key];
}

glm::vec2
Cel::Input::Input::MouseScroll() const
{
    return mouseScroll;
}

glm::vec2
Cel::Input::Input::MouseDelta() const
{
    return mouseDelta;
}

bool
Cel::Input::Input::MouseButtonUp(uint32_t button) const
{
    return !currentMouseButtonState[button] && previousMouseButtonState[button];
}

bool
Cel::Input::Input::MouseButtonDown(uint32_t button) const
{
    return currentMouseButtonState[button] && !previousMouseButtonState[button];
}

bool
Cel::Input::Input::MouseButtonHeld(uint32_t button) const
{
    return currentMouseButtonState[button] && previousMouseButtonState[button];
}

void
Cel::Input::Input::TickInput()
{
    // copy last frames input buffer into array
    previousKeyState = currentKeyState;
    previousMouseButtonState = currentMouseButtonState;

    mouseDelta = glm::vec2(0.0f);
    mouseScroll = glm::vec2(0.0f);
}

void
Cel::Input::ProcessInputEvents(Resource<Input>& input,
                               Resource<Running>& isRunning)
{

    input->TickInput();

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                isRunning->isRunning = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                input->currentKeyState[event.key.scancode] = true;
                break;
            case SDL_EVENT_KEY_UP:
                input->currentKeyState[event.key.scancode] = false;
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                input->mouseScroll = { event.wheel.x, event.wheel.y };
                break;
            case SDL_EVENT_MOUSE_MOTION:
                input->mouseDelta = { event.motion.xrel, event.motion.yrel };
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                input->currentMouseButtonState[event.button.button] = true;
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                input->currentMouseButtonState[event.button.button] = false;
                break;
            default:
                // Unimplemented events
                break;
        }
    }
}
