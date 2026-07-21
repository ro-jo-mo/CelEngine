#pragma once

#include "ecs/Schedule.h"

#include <chrono>
#include <typeindex>
#include <unordered_map>

namespace Cel {
/**
 * @brief Resource for time management
 * Automatically switches between fixed and dynamic time based on the current
 * schedule running
 */
class Time
{
  public:
    Time()
        : dynamicDeltaTime(0.01)
        , currentDelta(0.01)
        , lastDynamicUpdate(std::chrono::steady_clock::now())
    {
    }

    /**
     * @brief Get the current frame time in ms.
     * This will automatically switch between dynamic and fixed time steps based
     * on the current schedule running
     * @return
     */
    [[nodiscard]] float delta_time() const;

  private:
    template<typename Schedule>
    void register_schedule();
    /**
     * @brief Switch to fixed timestep
     */
    template<ScheduleEnum Enum>
    void switch_to_fixed();

    /**
     * @brief Switch to dynamic timestep
     */
    void switch_to_dynamic();

    /**
     * Check if a fixed update cycle is required
     * @return True if we need to run fixed update
     */
    template<ScheduleEnum Enum>
    bool is_fixed_update_required();

    /**
     * Tick the dynamic clock
     */
    void tick();

    /**
     * Tick the fixed update clock
     */
    template<ScheduleEnum Enum>
    void fixed_tick();

    // Delta time used for dynamic updates
    float dynamicDeltaTime;
    // Represents the current delta time we using, switches based on whether in
    // a fixed or dynamic execution schedule
    float currentDelta;

    // A mapping from schedules to their delta time
    std::unordered_map<std::type_index, float> fixedDeltas;

    // The time of the next update for a given schedule
    std::unordered_map<std::type_index, std::chrono::steady_clock::time_point>
        fixedUpdateIntervals;

    // Last clock tick for dynamic schedule
    std::chrono::steady_clock::time_point lastDynamicUpdate;

    // Allow the app to switch between timesteps and tick clocks without
    // exposing it publicly
    friend class App;
};

template<typename Schedule>
void
Time::register_schedule()
{
    if constexpr (Schedule::IsFixed) {
        fixedDeltas[typeid(typename Schedule::ScheduleEnum)] = Schedule::Tick;
        fixedUpdateIntervals[typeid(typename Schedule::ScheduleEnum)] =
            std::chrono::steady_clock::now();
    }
}

template<ScheduleEnum Enum>
void
Time::switch_to_fixed()
{
    currentDelta = fixedDeltas.at(typeid(Enum));
}

template<ScheduleEnum Enum>
bool
Time::is_fixed_update_required()
{
    using namespace std::chrono;

    const auto currentTime = steady_clock::now();

    const duration<float> delta =
        (fixedUpdateIntervals[typeid(Enum)] - currentTime);
    return delta.count() < 0;
}

template<ScheduleEnum Enum>
void
Time::fixed_tick()
{
    using namespace std::chrono;

    fixedUpdateIntervals[typeid(Enum)] += duration_cast<steady_clock::duration>(
        duration<float>(fixedDeltas[typeid(Enum)]));
}

}
