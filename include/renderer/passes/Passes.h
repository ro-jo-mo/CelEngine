#pragma once

#include "HandleAllocator.h"

namespace Cel::Renderer::RenderGraph {
class PassBuilder;
}
namespace Cel::Renderer::Passes {

// All passes run *after* the base pass. Used as a marker for existing resources
// with an existing state, i.e. we might need barriers and such
inline const auto basePass = HandleAllocator::allocate_pass("base_pass");

// The base state of any per frame resource. Simply marks that this resource
// does not need any synchronisation or transfer from its existing state, as it
// has no data
inline const auto nullPass = HandleAllocator::allocate_pass("null_pass");

// Presents the frame to the swapchain
inline const auto presentPass = HandleAllocator::allocate_pass("present_pass");

inline const auto drawImage = HandleAllocator::allocate_image("draw_image");

// The typical render passes I expect all programs to have are also stored here,
// to simplify includes

// Bind descriptors pass
inline const auto bindDescriptorsPass =
    HandleAllocator::allocate_pass("bind_descriptors_pass");
inline const auto sceneDataBuffer =
    HandleAllocator::allocate_buffer("scene_data_buffer");
inline const auto entityDataBuffer =
    HandleAllocator::allocate_buffer("entity_data_buffer");

// Asset server pass
inline const auto uploadAssetsPass =
    HandleAllocator::allocate_pass("upload_assets_pass");
inline const auto vertexBufferStaging =
    HandleAllocator::allocate_buffer("vertex_buffer_staging");
inline const auto indiceBufferStaging =
    HandleAllocator::allocate_buffer("indice_buffer_staging");
inline const auto materialBufferStaging =
    HandleAllocator::allocate_buffer("materials_buffer_staging");

}