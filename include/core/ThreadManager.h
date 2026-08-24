#pragma once
#include <cstdint>
#include <thread>

namespace Cel {

class ThreadManager
{
  public:
    static uint32_t total_threads();
};

}