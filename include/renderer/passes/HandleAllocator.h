#pragma once

#include "../render-graph/RenderGraphTypes.h"
#include "common/Handle.h"

#include <string>

namespace Cel::Renderer::Passes::HandleAllocator {

// Used for the static allocation of handles used by render passes
// It's assumed that a handle is allocated exactly once per pass resource, e.g.
// constexpr auto shadowTex = HandleAllocator::allocate_image("shadowTex");
// I made it thread safe just in case

Handle<AllocatedBuffer>
allocate_buffer(std::string name);

Handle<AllocatedImage>
allocate_image(std::string name);

Handle<RenderGraph::RenderPass>
allocate_pass(std::string name);

template<typename T>
Handle<T>
allocate_handle(const std::string& name);

template<>
Handle<AllocatedBuffer>
allocate_handle<AllocatedBuffer>(const std::string& name);

template<>
Handle<AllocatedImage>
allocate_handle<AllocatedImage>(const std::string& name);

std::string
get_name(Handle<AllocatedBuffer> handle);

std::string
get_name(Handle<AllocatedImage> handle);

std::string
get_name(Handle<RenderGraph::RenderPass> handle);

}