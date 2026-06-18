#include "../../include/core/Time.h"
#include <chrono>

using namespace Cel;
using namespace std::chrono;

float
Time::DeltaTime() const
{
    return currentDelta;
}

void
Time::SwitchToDynamic()
{
    currentDelta = dynamicDeltaTime;
}

void
Time::Tick()
{
    auto currentTime = steady_clock::now();
    duration<float> delta = (currentTime - lastDynamicUpdate);
    dynamicDeltaTime = delta.count();
    lastDynamicUpdate = currentTime;
}
