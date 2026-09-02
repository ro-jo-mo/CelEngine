#pragma once

namespace Cel::Renderer::Passes {

// Passes will create their own indirect buffers?
// Passes will add to the indirect mega buffer during their registration phase
// Then the mega buffer is uploaded in a pass, and then indirect passes can come
// after This pass registration simpyl needs to occur prior to the indirect
// buffer upload registration
// However, ideally as it's per frame it would be handled by the render graph
// Perhaps a version of the mega buffer that doesn't own a buffer?
// Really it's just reset each frame
// In this case I think it's fine that we rely on traditional scheduling to
// ensure our passes upload their indirect commands to the mega buffer before
// its uploaded during a pass

void
draw_meshes();

}