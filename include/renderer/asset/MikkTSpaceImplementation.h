#pragma once
#include <cstdint>
#include <mikktspace.h>
#include <vector>

namespace Cel::Renderer::Asset {
struct Vertex;
}
namespace Cel::Renderer::Asset::MikkTSpace {

struct MikkTData
{
    std::vector<Vertex>& vertices;
    const std::vector<uint32_t>& indices;
};

// An implementation of the mikktspace interface for my renderer
// m_pUserData should point to MikkTData object
// This requires that the mesh is de indexed
SMikkTSpaceInterface*
get_interface();

}