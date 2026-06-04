#pragma once

namespace Cel {
/**
 * @brief A trivial resource to check whether the engine loop should be running.
 * Change this to false to end the program
 */
class Running
{
  public:
    bool isRunning = true;
};
}