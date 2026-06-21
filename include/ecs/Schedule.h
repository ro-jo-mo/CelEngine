#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace Cel {

template<typename T>
concept ScheduleEnum =
    std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, uint32_t>;

/**
 * @brief Specify this schedule as a fixed update schedule, that ticks a set
 * number of times per second
 * @tparam Enum
 * @tparam TickRate The number of updates per second, i.e. TickRate 5 -> Run
 * Schedule every 200ms
 */
template<ScheduleEnum Enum, uint32_t TickRate>
struct FixedSchedule
{
    using ScheduleEnum = Enum;
    static constexpr uint32_t Tick = TickRate;
    static constexpr bool IsFixed = true;
};

template<ScheduleEnum Enum>
struct DynamicSchedule
{
    using ScheduleEnum = Enum;
    static constexpr bool IsFixed = false;
};

template<typename T>
struct IsSchedule : std::false_type
{};

template<ScheduleEnum Enum>
struct IsSchedule<DynamicSchedule<Enum>> : std::true_type
{};

template<ScheduleEnum Enum, uint32_t TickRate>
struct IsSchedule<FixedSchedule<Enum, TickRate>> : std::true_type
{};

// Move this to physics plugin at a later stage
enum class PhysicsUpdate : uint32_t
{
    First,
    PreUpdate,
    Update,
    PostUpdate,
    Last
};

enum class MainUpdate : uint32_t
{
    First,
    PreUpdate,
    Update,
    PostUpdate,
    Last
};

enum class Startup : uint32_t
{
    First,
    PreStart,
    Start,
    PostStart,
    Last
};

// There seems little reason to have TearDown be as complexly ordered as other
// schedules
enum class TearDown : uint32_t
{
    First,
    Middle,
    Last
};

enum class Render : uint32_t
{
    First,
    PreUpdate,
    Update,
    PostUpdate,
    Last
};

}
