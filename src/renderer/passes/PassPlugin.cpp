#include "renderer/passes/PassPlugin.h"

#include "renderer/passes/BasePasses.h"
#include "renderer/passes/DrawMeshes.h"

void
Cel::Renderer::Passes::PassPlugin::build(SystemScheduler scheduler,
                                         ResourceManager& resourceManager)
{
    // Add pass declaration & recording to ecs schedule
    // Certain passes actually need to occur in a specific cpu order,
    // particularly indirect command recording where it's needed to know what
    // entity data to upload for the scene

    scheduler.add_group(Render::First,
                        register_indirect_draw_data_pass,
                        register_draw_mesh_pass,
                        PassFriend::register_asset_upload_pass,
                        PassFriend::register_create_scene_data_pass);

    scheduler.add_chain(Render::Update,
                        create_indirect_draw_data,
                        PassFriend::create_and_bind_scene_data);

    scheduler.add_group(Render::Update, draw_mesh, PassFriend::upload_assets);
}
