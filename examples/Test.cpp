#include "core/App.h"
#include "core/CorePlugin.h"
#include "renderer/AssetServer.h"
#include "renderer/Camera.h"
#include "renderer/RenderPlugin.h"

#include <fastgltf/types.hpp>

using namespace Cel;

void
SpawnCamera(Resource<World>& world)
{
    world->Spawn(Renderer::Camera::Camera3d(90, 0.001, 1000));
}

struct RotateMe
{};

void
SpawnAsset(Resource<World>& world, Resource<Renderer::AssetServer>& server)
{
    auto handle = server->LoadAsset("../../examples/assets/horse.glb");

    // Spawn a parent class that we rotate for fun (test transform propagation)
    world->Spawn(RotateMe{}).WithChildren([&](ChildBuilder builder) {
        // astoundingly large horse
        // In order to view it we must bring it to a 100th scale
        auto child = builder.Spawn(Position{ 2, 0, 0 }, Scale{ 0.01 });
        server->AddAssetToEntity(child.Get(), handle, world);
    });
}

void
SpinIt(Query<With<RotateMe, Rotation>>& query, Resource<Time>& time)
{
    const auto rot = glm::angleAxis(time->DeltaTime(), glm::vec3(0, 1, 0));
    for (auto [_, rotation] : query) {
        rotation.rotation *= rot;
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