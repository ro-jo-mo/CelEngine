#pragma once

#include "VulkanTypes.h"

namespace Cel::Renderer::Queues {

// Why not a resource?

// Based on current assumptions, there is no reason we should ever change these
// values
// Obviously if the user for example changed the device used, they would
// but we could just restart the app in this case

// Really its just most convenient to access without needing to pull vk context each time

inline Queue graphics;
inline Queue compute;
inline Queue transfer;

}