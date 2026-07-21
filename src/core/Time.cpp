#include "../../include/core/Time.h"
#include <chrono>

using namespace Cel;
using namespace std::chrono;

float
Time::delta_time() const
{
    return currentDelta;
}

void
Time::switch_to_dynamic()
{
    currentDelta = dynamicDeltaTime;
}

void
Time::tick()
{
    auto currentTime = steady_clock::now();
    duration<float> delta = (currentTime - lastDynamicUpdate);
    dynamicDeltaTime = delta.count();
    lastDynamicUpdate = currentTime;
}
