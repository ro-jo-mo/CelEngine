#pragma once

#include <functional>
#include <iostream>

namespace Cel::Renderer {
template<typename Tag>
class DeletionQueue
{
  public:
    int name = 0;
    void Push(std::function<void()>&& func);
    void Flush();

  private:
    std::vector<std::function<void()>> queue;
};

template<typename Tag>
void
DeletionQueue<Tag>::Push(std::function<void()>&& func)
{
    queue.push_back(std::move(func));
}

template<typename Tag>
void
DeletionQueue<Tag>::Flush()
{
    // Deletion done in reverse order, i.e. newest items deleted first
    for (auto func = queue.rbegin(); func != queue.rend(); ++func) {
        (*func)();
    }
    queue.clear();
}

struct FinalCleanupTag
{};
using FinalCleanup = DeletionQueue<FinalCleanupTag>;

}