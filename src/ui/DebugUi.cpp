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

namespace {

bool
is_mouse_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_WHEEL:
            return true;
        default:
            return false;
    }
}

bool
is_keyboard_event(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        case SDL_EVENT_TEXT_INPUT:
        case SDL_EVENT_TEXT_EDITING:
            return true;
        default:
            return false;
    }
}

}

void
Cel::Ui::debug_ui_begin(Resource<DebugUi>& ui, Resource<Input::Input>& input)
{
    // Runs before input state is applied, so look for the toggle in the raw
    // events. The toggle key is consumed
    for (Input::InputEvent& inputEvent : input->events()) {
        const SDL_Event& event = inputEvent.event;
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
            event.key.scancode == ui->toggleKey) {
            ui->visible = !ui->visible;
            inputEvent.consumed = true;
        }
    }

    // Always feed ImGui so its internal key/mouse state stays consistent
    for (const Input::InputEvent& inputEvent : input->events()) {
        ImGui_ImplSDL3_ProcessEvent(&inputEvent.event);
    }

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (!ui->visible) {
        return;
    }

    // ImGui updates its capture flags in NewFrame from the events above, so
    // these reflect whether this frame's input landed on an ImGui window
    const ImGuiIO& io = ImGui::GetIO();
    for (Input::InputEvent& inputEvent : input->events()) {
        if ((io.WantCaptureMouse && is_mouse_event(inputEvent.event)) ||
            (io.WantCaptureKeyboard && is_keyboard_event(inputEvent.event))) {
            inputEvent.consumed = true;
        }
    }
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
        .after(Input::poll_input_events)
        .before(Input::apply_input_events);

    manager.add_system(Render::First, debug_ui_end)
        .after(Renderer::set_render_extent);

    manager.add_system(Render::PreUpdate,
                       Renderer::Passes::register_debug_ui_pass);
    manager.add_system(Render::PostUpdate,
                       Renderer::Passes::record_debug_ui_pass);

    // Before the renderer is destroyed
    manager.add_system(TearDown::First, teardown_imgui);
}
