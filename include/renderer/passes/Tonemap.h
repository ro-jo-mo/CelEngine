#pragma once

namespace Cel::Renderer::Passes {

// I need to add the draw image to the texture cache, ideally each frame (to
// accommodate swapchain resizing)
// vkCmdDraw(3,1,0)

void
register_tonemap();

}