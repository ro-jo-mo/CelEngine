#pragma once

#include "HandleAllocator.h"
namespace Cel::Renderer::Passes {

// All passes run *after* the base pass. Used as a marker for existing resources
// with an existing state, i.e. we might need barriers and such
inline auto basePass = HandleAllocator::allocate_pass("base_pass");

// The base state of any per frame resource. Simply marks that this resource
// does not need any synchronisation or transfer from its existing state, as it
// has no data
inline auto nullPass = HandleAllocator::allocate_pass("null_pass");

}