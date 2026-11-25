#pragma once
#include "core/Running.h"
#include "ecs/Resource.h"
#include "renderer/components/Window.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <array>
#include <glm/glm.hpp>
#include <span>
#include <vector>

namespace Cel::Input {

// A raw SDL event for this frame. UI layers (ImGui, RmlUi, ...) run between
// poll_input_events and apply_input_events and set `consumed` on any event
// they handled. Consumed events do not affect the gameplay facing state
struct InputEvent
{
    SDL_Event event;
    bool consumed = false;
};

class Input
{
  public:
    /**
     * @brief True if the user has just released the key
     * @param key
     * @return Has the key moved up this frame?
     */
    [[nodiscard]] bool key_up(SDL_Scancode key) const;

    /**
     * @brief True if the user just pressed this key
     * @param key
     * @return Has the key been pressed down this frame?
     */
    [[nodiscard]] bool key_down(SDL_Scancode key) const;
    /**
     * @brief True if this key has been pressed since last frame
     * @param key
     * @return Has the key been pressed down this frame and the last?
     */
    [[nodiscard]] bool key_held(SDL_Scancode key) const;

    /**
     * @brief Return the mouse movement of this frame
     * Raw input only
     * @return X for horizontal, Y for vertical
     */
    [[nodiscard]] glm::vec2 mouse_delta() const;
    /**
     * @brief Returns a vec2 of the scrolling done
     * @return x for horizontal scrolling, y for vertical scrolling
     */
    [[nodiscard]] glm::vec2 mouse_scroll() const;

    /**
     * @brief True if this mouse button was just released
     * @param button SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT etc
     * @return Has this button been released this frame
     */
    [[nodiscard]] bool mouse_button_up(uint32_t button) const;
    /**
     * @brief True if this mouse button was just pressed
     * @param button SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT etc
     * @return Has this button been pressed down this frame
     */
    [[nodiscard]] bool mouse_button_down(uint32_t button) const;
    /**
     * @brief True if this mouse button is held down
     * @param button SDL_BUTTON_LEFT, SDL_BUTTON_RIGHT etc
     * @return Has this button been pressed down this frame and the last
     */
    [[nodiscard]] bool mouse_button_held(uint32_t button) const;

    /**
     * @brief This frame's raw events. Mainly for UI layers
     */
    [[nodiscard]] std::span<InputEvent> events();

  private:
    void tick_input();

    // Whether the key is currently down, as of the last event this frame
    std::array<bool, SDL_SCANCODE_COUNT> keyState{};

    // Edges seen this frame.
    // The same key can be pressed and released in a single frame so we must
    // track both
    std::array<bool, SDL_SCANCODE_COUNT> keyPressedThisFrame{};
    std::array<bool, SDL_SCANCODE_COUNT> keyReleasedThisFrame{};

    // Codes go 1 - 5 in SDL_MouseButtonFlags
    std::array<bool, 6> currentMouseButtonState{};
    std::array<bool, 6> previousMouseButtonState{};
    glm::vec2 mouseScroll;
    glm::vec2 mouseDelta;

    std::vector<InputEvent> frameEvents;

    friend void poll_input_events(Resource<Input>& input,
                                  Resource<Running>& isRunning,
                                  Resource<Renderer::Window>& window);
    friend void apply_input_events(Resource<Input>& input);
};

void
poll_input_events(Resource<Input>& input,
                  Resource<Running>& isRunning,
                  Resource<Renderer::Window>& window);

void
apply_input_events(Resource<Input>& input);

}