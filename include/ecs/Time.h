#pragma once

#include "Resource.h"
#include <chrono>


namespace Cel {
  class Time : public Resource {
  public:
    explicit Time(float fixedTimeStep);

    float DeltaTime() const;

    void SwitchToFixed();

    void SwitchToDynamic();

    bool FixedUpdateRequired();

    void Tick();

    void FixedTick();

  private:
    float dynamicDeltaTime;
    float fixedDeltaTime;
    float currentDelta;
    std::chrono::steady_clock::time_point nextFixedUpdate;
    std::chrono::steady_clock::time_point lastUpdate;
  };
}
