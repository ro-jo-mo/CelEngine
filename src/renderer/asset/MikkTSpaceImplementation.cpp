#include "renderer/asset/MikkTSpaceImplementation.h"

#include "renderer/asset/AssetTypes.h"

#include <cassert>

using Cel::Renderer::Asset::Vertex;
using Cel::Renderer::Asset::MikkTSpace::MikkTData;

namespace {

MikkTData&
get_data(const SMikkTSpaceContext* pContext)
{
    return *static_cast<MikkTData*>(pContext->m_pUserData);
}

// MikkTSpace addresses vertices by (face, corner of face), so we map that back
// to our indexed vertex through the index buffer
Vertex&
get_vertex(const SMikkTSpaceContext* pContext, const int iFace, const int iVert)
{
    auto& data = get_data(pContext);

    const size_t corner =
        static_cast<size_t>(iFace) * 3 + static_cast<size_t>(iVert);

    assert(corner < data.indices.size());
    assert(data.indices[corner] < data.vertices.size());

    return data.vertices[data.indices[corner]];
}

// Returns the number of faces (triangles/quads) on the mesh to be processed.
int
get_num_faces(const SMikkTSpaceContext* pContext)
{
    return static_cast<int>(get_data(pContext).indices.size() / 3);
}

// Returns the number of vertices on face number iFace
// iFace is a number in the range {0, 1, ..., getNumFaces()-1}
int
get_num_vertices_of_face(const SMikkTSpaceContext* pContext, const int iFace)
{
    // Triangles only
    return 3;
}

// returns the position/normal/texcoord of the referenced face of vertex number
// iVert. iVert is in the range {0,1,2} for triangles and {0,1,2,3} for quads.
void
get_position(const SMikkTSpaceContext* pContext,
             float fvPosOut[],
             const int iFace,
             const int iVert)
{
    const auto& position = get_vertex(pContext, iFace, iVert).position;

    fvPosOut[0] = position.x;
    fvPosOut[1] = position.y;
    fvPosOut[2] = position.z;
}

void
get_normal(const SMikkTSpaceContext* pContext,
           float fvNormOut[],
           const int iFace,
           const int iVert)
{
    const auto& normal = get_vertex(pContext, iFace, iVert).normal;

    fvNormOut[0] = normal.x;
    fvNormOut[1] = normal.y;
    fvNormOut[2] = normal.z;
}

void
get_tex_coord(const SMikkTSpaceContext* pContext,
              float fvTexcOut[],
              const int iFace,
              const int iVert)
{
    const auto& vertex = get_vertex(pContext, iFace, iVert);

    fvTexcOut[0] = vertex.uvX;
    fvTexcOut[1] = vertex.uvY;
}

// either (or both) of the two setTSpace callbacks can be set.
// The call-back m_setTSpaceBasic() is sufficient for basic normal mapping.

// This function is used to return the tangent and fSign to the application.
// fvTangent is a unit length vector.
// For normal maps it is sufficient to use the following simplified version of
// the bitangent which is generated at pixel/vertex level. bitangent = fSign *
// cross(vN, tangent); Note that the results are returned unindexed. It is
// possible to generate a new index list But averaging/overwriting tangent
// spaces by using an already existing index list WILL produce INCRORRECT
// results. DO NOT! use an already existing index list.
void
set_t_space_basic(const SMikkTSpaceContext* pContext,
                  const float fvTangent[],
                  const float fSign,
                  const int iFace,
                  const int iVert)
{
    get_vertex(pContext, iFace, iVert).tangent = {
        fvTangent[0], fvTangent[1], fvTangent[2], fSign
    };
}

SMikkTSpaceInterface mikkInterface = {
    .m_getNumFaces = get_num_faces,
    .m_getNumVerticesOfFace = get_num_vertices_of_face,
    .m_getPosition = get_position,
    .m_getNormal = get_normal,
    .m_getTexCoord = get_tex_coord,
    .m_setTSpaceBasic = set_t_space_basic,
    // add when needed if ever needed probably not
    .m_setTSpace = nullptr
};

}

SMikkTSpaceInterface*
Cel::Renderer::Asset::MikkTSpace::get_interface()
{
    return &mikkInterface;
}
