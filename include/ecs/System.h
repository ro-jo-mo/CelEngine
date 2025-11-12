#pragma once

namespace Cel {
  class System {
  public:
    virtual void Execute() = 0;

    virtual ~System() = default;
  };
}
