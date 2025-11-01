#pragma once

namespace Cel {
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
