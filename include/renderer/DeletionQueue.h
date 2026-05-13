#pragma once

#include <functional>
#include <iostream>

namespace Cel::Renderer {
namespace Detail {
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
struct PerFrameCleanupTag
{};
struct GenericTag
{};
}
// The need for distinct types here is so I can use these as resources
// Resource<FinalCleanup> & Resource<PerFrameCleanup> etc should be distinctive
using FinalCleanup = Detail::DeletionQueue<Detail::FinalCleanupTag>;
using PerFrameCleanup = Detail::DeletionQueue<Detail::PerFrameCleanupTag>;
using DeletionQueue = Detail::DeletionQueue<Detail::GenericTag>;
}