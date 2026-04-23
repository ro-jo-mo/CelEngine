#pragma once

namespace Cel {
  /**
   * @brief An enum for schedule states
   */
  enum Schedule : std::size_t {
    PreStartup,
    Startup,
    PostStartup,
    PreUpdate,
    Update,
    PostUpdate,
    PreFixedUpdate,
    FixedUpdate,
    PostFixedUpdate,
    SIZE
  };
}
