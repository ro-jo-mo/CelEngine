#include "../include/renderer/RenderPlugin.h"
#include "../include/renderer/asset/AssetServer.h"
#include "core/App.h"
#include "core/CorePlugin.h"
#include "renderer/Camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "input/Input.h"
#include "renderer/Window.h"
#include "renderer/passes/PassPlugin.h"
#include "renderer/render-graph/RenderGraphPlugin.h"

#include <fastgltf/types.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace Cel;

struct RotateMe
{};

struct PlayerCamera
{
    float totalX = 0;
    float totalY = 0;
};

void
spawn_camera(Resource<World>& world)
{
    world->spawn(Renderer::Camera::Camera3d(90, 0.1, 1000), PlayerCamera{});
}

void
spawn_asset(Resource<World>& world,
            Resource<Renderer::Asset::AssetServer>& server)
{
    auto handle = server->load_gltf("../../examples/assets/horse.glb", {});

    // Spawn a parent class that we rotate for fun (test transform propagation)
    world->spawn(RotateMe{}).with_children([&](ChildBuilder builder) {
        // astoundingly large horse
        // In order to view it we must bring it to a 100th scale
        auto child =
            builder.spawn(Position{ 0, 0, -1 }, Scale{ 0.01 }, RotateMe{});
        server->add_scene_to_entity(child.get(), handle);
    });
    server->add_scene_to_entity(
        world->spawn(Position{ 0, 0, 10 }, Scale{ 0.01 }).get(), handle);
}

void
spin_it(Query<With<RotateMe, Rotation>>& query, Resource<Time>& time)
{
    const auto rot = glm::angleAxis(time->delta_time(), glm::vec3(0, 1, 0));
    for (auto [_, rotation] : query) {
        rotation.rotation *= rot;
    }
}

void
camera_controller(
    Query<With<Renderer::Camera, PlayerCamera, Rotation, Position>>& query,
    Resource<Input::Input>& input,
    Resource<Renderer::Window>& window,
    Resource<Time>& time)
{
    constexpr float SENSITIVITY = 0.07;
    constexpr float SPEED = 3.0;

    auto mouse = input->mouse_delta() * SENSITIVITY;
    auto scroll = input->mouse_scroll();

    auto rotation = glm::angleAxis(mouse.x, glm::vec3(0, 1, 0));
    rotation *= glm::angleAxis(mouse.y, glm::vec3(1, 0, 0));

    auto translation = glm::vec3(0.0);

    if (input->key_held(SDL_SCANCODE_A)) {
        translation.x -= 1;
    }
    if (input->key_held(SDL_SCANCODE_D)) {
        translation.x += 1;
    }
    if (input->key_held(SDL_SCANCODE_W)) {
        translation.z += 1;
    }
    if (input->key_held(SDL_SCANCODE_S)) {
        translation.z -= 1;
    }
    if (input->key_held(SDL_SCANCODE_SPACE)) {
        translation.y += 1;
    }
    if (input->key_held(SDL_SCANCODE_LCTRL)) {
        translation.y -= 1;
    }

    translation *= SPEED;

    for (auto [cam, ms, rot, pos] : query) {
        ms.totalX += mouse.x * SENSITIVITY;
        ms.totalY += mouse.y * SENSITIVITY;

        rot.rotation = glm::angleAxis(ms.totalX, glm::vec3(0.0, 1.0, 0)) *
                       glm::angleAxis(ms.totalY, glm::vec3(1.0, 0, 0));

        cam.fov -= scroll.y * 0.1;

        pos.position += rot.rotation * translation * time->delta_time();
    }

    if (input->mouse_button_down(SDL_BUTTON_RIGHT)) {
        static bool toggle = false;
        toggle = !toggle;
        SDL_SetWindowRelativeMouseMode(window->window, toggle);
    }
}

class MyPlugin : public Plugin
{
  public:
    void build(SystemScheduler scheduler,
               ResourceManager& resourceManager) override
    {
        scheduler.add_system(Startup::Start, spawn_camera);
        scheduler.add_system(Startup::Start, spawn_asset);

        scheduler.add_system(MainUpdate::Update, spin_it);
        scheduler.add_system(MainUpdate::Last, camera_controller);
    }
};

int
main()
{
    static_assert(sizeof(GlobalTransform) > 0);

    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<Renderer::RenderPlugin>()
        .add_plugin<Renderer::RenderGraph::RenderGraphPlugin>()
        .add_plugin<Renderer::Passes::PassPlugin>()
        .add_plugin<MyPlugin>();
    ecs.start<Startup>()
        .loop<FixedSchedule<PhysicsUpdate, 50>,
              DynamicSchedule<MainUpdate>,
              DynamicSchedule<Render>>()
        .end<TearDown>();

    return 0;
}