#include "renderer/Draw.h"

#include "renderer/VulkanHelpers.h"

void
Cel::Renderer::DrawGeometry::Run(Resource<VulkanContext>& context,
                                 Resource<DrawImage>& drawImage,
                                 Resource<VkPipeline>& pipeline,
                                 Resource<RenderExtent>& renderExtent)
{
    VkRenderingAttachmentInfo colourAttachment =
        Initialisers::AttachmentInfo(drawImage->imageView,
                                     nullptr,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = Initialisers::RenderingInfo(
        renderExtent->extent, &colourAttachment, nullptr);

    // Command buffer????

    // What I need:
    // Synchronisation primitives
    // Command buffers -> Command pools
    // All bundled into frame data
}

void
Cel::Renderer::SetRenderExtent::Run(Resource<RenderExtent>& renderExtent,
                                    Resource<DrawImage>& drawImage,
                                    Resource<Swapchain>& swapchain)
{
    renderExtent->extent.height =
        std::min(swapchain->extent.height, drawImage->imageExtent.height) *
        renderExtent->renderScale;
    renderExtent->extent.width =
        std::min(swapchain->extent.width, drawImage->imageExtent.width) *
        renderExtent->renderScale;
}
