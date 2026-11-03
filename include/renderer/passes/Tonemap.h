#pragma once

namespace Cel::Renderer::Passes {

// I need to add the draw image to the texture cache, ideally each frame (to
// accommodate swapchain resizing)

void
register_tonemap();

}