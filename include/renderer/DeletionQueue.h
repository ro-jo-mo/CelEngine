#pragma once
#include <functional>

namespace Cel::Renderer {
class DeletionQueue
{
  public:
    void Push(std::function<void()>&& func);
    void Flush();

  private:
    std::vector<std::function<void()>> queue;
};

class FinalCleanup : public DeletionQueue
{};
}