#pragma once

#include <functional>

#include "SystemAllocator.h"


namespace Cel {
  /**
   * @brief Abstract base class for game systems
   * @tparam Parameters A list of queries and resources required by this system
   */
  template<typename... Parameters>
  class System {
  public:
    virtual ~System() = default;

    /**
     * @brief Registers this system, creating a new function with the required data captured
     * @param system A system pointer, required for polymorphism
     * @param allocator Resource allocator for system
     * @return A function that runs this system with its required params. Requires no inputs.
     */
    static std::function<void()> Register(std::shared_ptr<System> system, SystemAllocator &allocator);

    /**
     * @brief Virtual method for running the system.
     * @param params Queries, resources needed to run
     */
    virtual void Run(Parameters &... params) = 0;
  };

  template<typename... Parameters>
  std::function<void()> System<Parameters...>::Register(std::shared_ptr<System> system, SystemAllocator &allocator) {
    auto args = std::tuple<Parameters &...>(allocator.Register<Parameters>()...);
    auto run = [system](auto &... _args) { system->Run(_args...); };

    return [run,args = std::move(args)]() {
      std::apply(run, args);
    };
  }
}
