#include "ui/DebugUi.h"

#include "input/Input.h"
#include "renderer/components/RenderTypes.h"
#include "renderer/components/Window.h"
#include "renderer/passes/DebugUiPass.h"
#include "renderer/resource-management/Queues.h"
#include "renderer/vulkan/VulkanHelpers.h"
#include "renderer/vulkan/VulkanSetup.h"

#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

namespace {

void
check_vk_result(VkResult error)
{
    Cel::Renderer::vk_check(error);
}

}

void
Cel::Ui::init_imgui(Resource<Renderer::VulkanContext>& context,
                    Resource<Renderer::Window>& window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForVulkan(window->window);

    // Static so the pointer stays alive for lifetime of ui
    static constexpr VkFormat ATTACHMENT_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_3;
    info.Instance = context->instance;
    info.PhysicalDevice = context->gpu;
    info.Device = context->device;
    info.QueueFamily = Renderer::Queues::graphics.family;
    info.Queue = Renderer::Queues::graphics.queue;

    info.DescriptorPoolSize = 16;

    info.MinImageCount = 2;
    info.ImageCount = Renderer::FRAMES_IN_FLIGHT;
    info.UseDynamicRendering = true;
    info.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &ATTACHMENT_FORMAT,
    };
    info.CheckVkResultFn = check_vk_result;

    ImGui_ImplVulkan_Init(&info);
}

void
Cel::Ui::debug_ui_begin(Resource<DebugUi>& ui, Resource<Input::Input>& input)
{
    if (input->key_down(ui->toggleKey)) {
        ui->visible = !ui->visible;
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void
Cel::Ui::debug_ui_end(Resource<Renderer::RenderExtent>& extent)
{
    ImGui::Render();

    ImDrawData* drawData = ImGui::GetDrawData();

    // ImGui does it's own command recording and management, so it's handled
    // outside the rendergraph here
    if (drawData->Textures != nullptr) {
        for (ImTextureData* texture : *drawData->Textures) {
            if (texture->Status != ImTextureStatus_OK) {
                ImGui_ImplVulkan_UpdateTexture(texture);
            }
        }
    }

    // The SDL backend sizes the display from the window, but we draw into the
    // final image, which is window pixels * render scale. Map display
    // coordinates onto the final image so scissor rects are correct. Only the
    // draw data is changed, so font rasterisation density is unaffected
    if (drawData->DisplaySize.x > 0 && drawData->DisplaySize.y > 0) {
        drawData->FramebufferScale = {
            static_cast<float>(extent->extent.width) / drawData->DisplaySize.x,
            static_cast<float>(extent->extent.height) / drawData->DisplaySize.y
        };
    }
}

void
Cel::Ui::teardown_imgui(Resource<Renderer::VulkanContext>& context)
{
    vkDeviceWaitIdle(context->device);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}

void
Cel::Ui::DebugUiPlugin::build(PluginManager& manager)
{
    manager.insert_resource<DebugUi>(false, toggleKey);

    manager.add_system(Startup::PreStart, init_imgui)
        .after(Renderer::initialise_renderer);

    manager.add_system(MainUpdate::First, debug_ui_begin)
        .after(Input::process_input_events);

    manager.add_system(Render::First, debug_ui_end)
        .after(Renderer::set_render_extent);

    manager.add_system(Render::PreUpdate, Renderer::Passes::register_debug_ui_pass);
    manager.add_system(Render::PostUpdate, Renderer::Passes::record_debug_ui_pass);

    // Before the renderer is destroyed
    manager.add_system(TearDown::First, teardown_imgui);
}
