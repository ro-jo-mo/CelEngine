#pragma once

#include <functional>

#include "SystemAllocator.h"


namespace Cel {
  template<typename... Parameters>
  class System {
  public:
    virtual ~System() = default;

    static std::function<void()> Register(std::shared_ptr<System> system, SystemAllocator &allocator);

    virtual void Run(Parameters &... params) = 0;
  };

  template<typename... Parameters>
  std::function<void()> System<Parameters...>::Register(std::shared_ptr<System> system, SystemAllocator &allocator) {
    std::cout << "Register" << std::endl;
    auto args = std::tuple<Parameters &...>(allocator.Register<Parameters>()...);
    auto run = [system](auto &... _args) { system->Run(_args...); };

    return [run,args = std::move(args)]() {
      std::apply(run, args);
    };
  }
}
