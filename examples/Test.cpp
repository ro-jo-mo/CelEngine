#include "core/App.h"
#include "core/CorePlugin.h"
#include "renderer/AssetServer.h"
#include "renderer/Camera.h"
#include "renderer/RenderPlugin.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "input/Input.h"
#include "renderer/Window.h"

#include <fastgltf/types.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace Cel;

struct RotateMe
{};

struct MyAsset
{};

struct PlayerCamera
{
    float totalX = 0;
    float totalY = 0;
};

void
SpawnCamera(Resource<World>& world)
{
    world->spawn(Renderer::Camera::Camera3d(90, 0.1, 1000), PlayerCamera{});
}

void
SpawnAsset(Resource<World>& world, Resource<Renderer::AssetServer>& server)
{
    auto handle = server->load_gltf_asset("../../examples/assets/horse.glb");

    // Spawn a parent class that we rotate for fun (test transform propagation)
    world->spawn(RotateMe{}).with_children([&](ChildBuilder builder) {
        // astoundingly large horse
        // In order to view it we must bring it to a 100th scale
        auto child = builder.spawn(
            Position{ 0, 0, -1 }, Scale{ 0.01 }, MyAsset{}, RotateMe{});
        server->add_asset_to_entity(child.get(), handle, world);
    });
    server->add_asset_to_entity(
        world->spawn(Position{ 0, 0, 5 }, Scale{ 0.01 }).get(), handle, world);
}

void
SpinIt(Query<With<RotateMe, Rotation>>& query, Resource<Time>& time)
{
    const auto rot = glm::angleAxis(time->delta_time(), glm::vec3(0, 1, 0));
    for (auto [_, rotation] : query) {
        rotation.rotation *= rot;
    }
}

void
CameraController(
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
    void build(Scheduler scheduler, ResourceManager& resourceManager) override
    {
        scheduler.add_system(Startup::Start, SpawnCamera);
        scheduler.add_system(Startup::Start, SpawnAsset);
        scheduler.add_system(MainUpdate::Update, SpinIt);
        scheduler.add_system(MainUpdate::Last, CameraController);
    }
};

int
main()
{
    App ecs;
    ecs.add_plugin<CorePlugin>()
        .add_plugin<Renderer::RenderPlugin>()
        .add_plugin<MyPlugin>();
    ecs.start<Startup>()
        .loop<FixedSchedule<PhysicsUpdate, 50>,
              DynamicSchedule<MainUpdate>,
              DynamicSchedule<Render>>()
        .end<TearDown>();

    return 0;
}