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
    world->Spawn(Renderer::Camera::Camera3d(90, 0.1, 1000), PlayerCamera{});
}

void
SpawnAsset(Resource<World>& world, Resource<Renderer::AssetServer>& server)
{
    auto handle = server->LoadGltfAsset("../../examples/assets/horse.glb");

    // Spawn a parent class that we rotate for fun (test transform propagation)
    world->Spawn(RotateMe{}).WithChildren([&](ChildBuilder builder) {
        // astoundingly large horse
        // In order to view it we must bring it to a 100th scale
        auto child = builder.Spawn(
            Position{ 0, 0, -1 }, Scale{ 0.01 }, MyAsset{}, RotateMe{});
        server->AddAssetToEntity(child.Get(), handle, world);
    });
    server->AddAssetToEntity(
        world->Spawn(Position{ 0, 0, 5 }, Scale{ 0.01 }).Get(), handle, world);
}

void
SpinIt(Query<With<RotateMe, Rotation>>& query, Resource<Time>& time)
{
    const auto rot = glm::angleAxis(time->DeltaTime(), glm::vec3(0, 1, 0));
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

    auto mouse = input->MouseDelta() * SENSITIVITY;
    auto scroll = input->MouseScroll();

    auto rotation = glm::angleAxis(mouse.x, glm::vec3(0, 1, 0));
    rotation *= glm::angleAxis(mouse.y, glm::vec3(1, 0, 0));

    auto translation = glm::vec3(0.0);

    if (input->KeyHeld(SDL_SCANCODE_A)) {
        translation.x -= 1;
    }

    if (input->KeyHeld(SDL_SCANCODE_D)) {
        translation.x += 1;
    }

    if (input->KeyHeld(SDL_SCANCODE_W)) {
        translation.z += 1;
    }
    if (input->KeyHeld(SDL_SCANCODE_S)) {
        translation.z -= 1;
    }
    if (input->KeyHeld(SDL_SCANCODE_SPACE)) {
        translation.y += 1;
    }
    if (input->KeyHeld(SDL_SCANCODE_LCTRL)) {
        translation.y -= 1;
    }

    translation *= SPEED;

    for (auto [cam, ms, rot, pos] : query) {
        ms.totalX += mouse.x * SENSITIVITY;
        ms.totalY += mouse.y * SENSITIVITY;

        rot.rotation = glm::angleAxis(ms.totalX, glm::vec3(0.0, 1.0, 0)) *
                       glm::angleAxis(ms.totalY, glm::vec3(1.0, 0, 0));

        cam.fov -= scroll.y * 0.1;

        pos.position += rot.rotation * translation * time->DeltaTime();
    }

    if (input->MouseButtonDown(SDL_BUTTON_RIGHT)) {
        static bool toggle = false;
        toggle = !toggle;
        SDL_SetWindowRelativeMouseMode(window->window, toggle);
    }
}

class MyPlugin : public Plugin
{
  public:
    void Build(Scheduler scheduler, ResourceManager& resourceManager) override
    {
        scheduler.AddSystem(Startup::Start, SpawnCamera);
        scheduler.AddSystem(Startup::Start, SpawnAsset);
        scheduler.AddSystem(MainUpdate::Update, SpinIt);
        scheduler.AddSystem(MainUpdate::Last, CameraController);
    }
};

int
main()
{
    App ecs;
    ecs.AddPlugin<CorePlugin>()
        .AddPlugin<Renderer::RenderPlugin>()
        .AddPlugin<MyPlugin>();
    ecs.Start<Startup>()
        .Loop<FixedSchedule<PhysicsUpdate, 50>,
              DynamicSchedule<MainUpdate>,
              DynamicSchedule<Render>>()
        .End<TearDown>();

    return 0;
}