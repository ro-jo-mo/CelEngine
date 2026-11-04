#include "renderer/passes/PassPlugin.h"

#include "core/Transform.h"
#include "renderer/Camera.h"
#include "renderer/SceneData.h"
#include "renderer/passes/BasePasses.h"
#include "renderer/passes/DrawMeshes.h"
#include "renderer/passes/DrawSkybox.h"
#include "renderer/passes/PresentPass.h"

void
Cel::Renderer::Passes::PassPlugin::build(PluginManager& manager)
{
    // Add pass declaration & recording to ecs schedule
    // Certain passes actually need to occur in a specific cpu order,
    // particularly indirect command recording where it's needed to know what
    // entity data to upload for the scene

    manager.insert_resource<SceneData>();

    manager.insert_resource<SkyboxData>();

    manager.add_system(Startup::Last, create_skybox_data);

    manager.add_system(Render::First, PassFriend::register_asset_upload_pass);

    manager.add_group(Render::PreUpdate,
                        register_skybox_pass,
                        register_indirect_draw_data_pass,
                        register_draw_mesh_pass,
                        PassFriend::register_create_scene_data_pass,
                        register_present_pass);

    manager.add_chain(Render::PostUpdate,
                        create_indirect_draw_data,
                        PassFriend::create_scene_data);

    manager.add_group(Render::PostUpdate,
                        draw_mesh,
                        PassFriend::upload_assets,
                        draw_skybox,
                        record_present_pass);
}
