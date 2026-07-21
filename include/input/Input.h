#pragma once
#include "core/Running.h"
#include "ecs/Resource.h"

#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_scancode.h>
#include <array>
#include <glm/glm.hpp>

namespace Cel::Input {

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

  private:
    void TickInput();

    std::array<bool, SDL_SCANCODE_COUNT> currentKeyState{};
    std::array<bool, SDL_SCANCODE_COUNT> previousKeyState{};

    // Only 5 codes from SDL_MouseButtonFlags
    std::array<bool, 5> currentMouseButtonState{};
    std::array<bool, 5> previousMouseButtonState{};

    glm::vec2 mouseScroll;
    glm::vec2 mouseDelta;

    friend void process_input_events(Resource<Input>& input,
                                   Resource<Running>& isRunning);
};

void
process_input_events(Resource<Input>& input, Resource<Running>& isRunning);

}