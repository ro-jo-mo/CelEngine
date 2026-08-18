#pragma once
#include "common/Handle.h"

#include <string>
#include <unordered_map>

namespace Cel::Renderer::HandleAllocator {

struct AllocatedBuffer;

// Used for the static allocation of handles used by render passes
// It's assumed that a handle is allocated exactly once per pass resource, e.g.
// constexpr auto shadowTex = HandleAllocator::allocate_image("shadowTex");



static Handle<AllocatedBuffer>
allocate_buffer(std::string name);

static Handle<AllocatedBuffer>
allocate_image(std::string name);

static Handle<AllocatedBuffer>
allocate_pass(std::string name);

}