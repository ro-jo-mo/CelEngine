#include "renderer/CleanupRenderer.h"
void
Cel::Renderer::CleanupRenderer::Run(Resource<FinalCleanup>& cleanup)
{
    cleanup->Flush();
}
