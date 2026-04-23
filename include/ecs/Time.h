#pragma once

#include "Resource.h"
#include <chrono>


namespace Cel {
  /**
   * @brief Resource for time management
   * Automatically switches between fixed and dynamic time based on the current schedule running
   */
  class Time {
  public:
    explicit Time(float fixedTimeStep);

    /**
     * @brief Get the current frame time in ms.
     * This will automatically switch between dynamic and fixed time steps based on the current schedule running
     * @return
     */
    float DeltaTime() const;

    /**
     * @brief Switch to fixed timestep
     */
    void SwitchToFixed();

    /**
     * @brief Switch to dynamic timestep
     */
    void SwitchToDynamic();

    /**
     * Check if a fixed update cycle is required
     * @return True if we need to run fixed update
     */
    bool FixedUpdateRequired() const;

    /**
     * Tick the dynamic clock
     */
    void Tick();

    /**
     * Tick the fixed update clock
     */
    void FixedTick();

  private:
    float dynamicDeltaTime;
    float fixedDeltaTime;
    float currentDelta;
    std::chrono::steady_clock::time_point nextFixedUpdate;
    std::chrono::steady_clock::time_point lastUpdate;
  };
}
