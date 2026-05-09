#include "renderer/DeletionQueue.h"

void
Cel::Renderer::DeletionQueue::Push(std::function<void()>&& func)
{
    queue.push_back(std::move(func));
}
void
Cel::Renderer::DeletionQueue::Flush()
{
    // Deletion done in reverse order, i.e. newest items deleted first
    for (size_t i = queue.size(); i > 0; ++i) {
        queue[i]();
    }
    queue.clear();
}
