#pragma once

namespace Cel {
/**
 * @brief An enum for schedule states
 */
enum Schedule : std::size_t
{
    PreStartup,
    Startup,
    PostStartup,

    PreFixedUpdate,
    FixedUpdate,
    PostFixedUpdate,

    PreUpdate,
    Update,
    PostUpdate,

    FinalUpdate,
    Render,

    SIZE
};
}
