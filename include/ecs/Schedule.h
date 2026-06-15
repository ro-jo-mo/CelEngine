#pragma once

namespace Cel {

enum class UpdateType : uint8_t
{
    FixedUpdate,
    DynamicUpdate,
    OneShot
};

// Move this to physics plugin at a later stage
enum class PhysicsUpdate : uint8_t
{
    First,
    Pre,
    Update,
    Post,
    Final
};

enum class MainUpdate : uint8_t
{
    First,
    Pre,
    Update,
    Post,
    Final
};

// Add schedule
// typeid(PhysicsUpdate)

}
