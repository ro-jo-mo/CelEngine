#include "renderer/passes/DebugUiPass.h"

#include "renderer/passes/Passes.h"
#include "renderer/render-graph/PassBuilder.h"
#include "renderer/render-graph/PassServer.h"
#include "renderer/render-graph/RenderGraph.h"
#include "renderer/resource-management/Queues.h"
#include "renderer/vulkan/VulkanUtils.h"
#include "ui/DebugUi.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

void
Cel::Renderer::Passes::register_debug_ui_pass(
    Resource<Ui::DebugUi>& ui,
    Resource<RenderExtent>& extent,
    Resource<RenderGraph::Graph>& graph)
{
    const ImDrawData* drawData = ImGui::GetDrawData();

    bool active = ui->visible && drawData != nullptr &&
                  drawData->TotalVtxCount > 0 && extent->extent.width > 0 &&
                  extent->extent.height > 0;

    if (!active) {
        return;
    }

    auto pass =
        RenderGraph::PassBuilder(Passes::debugUiPass)
            .set_queue(Queues::graphics)
            .write_image(Passes::finalImage,
                         VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                         VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .build();

    graph->add_pass(pass).after(Passes::tonemapPass);
}

void
Cel::Renderer::Passes::record_debug_ui_pass(
    ParallelResource<RenderGraph::PassServer>& passServer)
{
    VkCommandBuffer cmd;
    {
        cmd = passServer.write()->get_cmd_buffer(Passes::debugUiPass);
    }

    // We expect this if the pass is never added
    if (cmd == VK_NULL_HANDLE) {
        return;
    }

    auto access = passServer.partial_read();

    auto guard = Utils::begin_rendering_no_depth(
        cmd,
        access->get_resource(Passes::finalImage).imageView,
        access->get_extent());

    // ImGui updates its textures itself, so this is just graphics work.
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
