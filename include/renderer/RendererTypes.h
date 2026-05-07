#pragma once
#include <deque>
#include <functional>

namespace Cel::Renderer {
#define VK_CHECK(x)                                                            \
    do {                                                                       \
        VkResult err = x;                                                      \
        if (err) {                                                             \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err));     \
            abort();                                                           \
        }                                                                      \
    } while (0)
class DeletionQueue
{
  public:
    void Push(std::function<void()>&& func);
    void Flush();

  private:
    std::vector<std::function<void()>> queue;
};
}