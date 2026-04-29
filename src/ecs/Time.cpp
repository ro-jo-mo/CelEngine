#include <chrono>
#include <ecs/Time.h>

using namespace Cel;

Time::Time(const float fixedTimeStep)
  : dynamicDeltaTime(0.01)
    , fixedDeltaTime(fixedTimeStep)
    , currentDelta(0.01)
    , nextFixedUpdate(std::chrono::steady_clock::now())
    , lastUpdate(std::chrono::steady_clock::now()) {
}

float
Time::DeltaTime() const {
  return currentDelta;
}

void
Time::SwitchToFixed() {
  currentDelta = fixedDeltaTime;
}

void
Time::SwitchToDynamic() {
  currentDelta = dynamicDeltaTime;
}

bool
Time::FixedUpdateRequired() const {
  const auto currentTime = std::chrono::steady_clock::now();
  const std::chrono::duration<float> delta = (nextFixedUpdate - currentTime);
  return delta.count() < 0;
}

void
Time::Tick() {
  auto currentTime = std::chrono::steady_clock::now();
  std::chrono::duration<float> delta = (currentTime - lastUpdate);
  dynamicDeltaTime = delta.count();
  lastUpdate = currentTime;
}

void Time::FixedTick() {
  using namespace std::chrono;
  nextFixedUpdate += duration_cast<steady_clock::duration>(
    duration<float>(fixedDeltaTime));
}
