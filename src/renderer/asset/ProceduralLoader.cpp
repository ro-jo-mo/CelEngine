#include "renderer/asset/ProceduralLoader.h"

#include "common/Misc.h"
#include "core/Error.h"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <initializer_list>

Cel::Handle<Cel::Renderer::Asset::Primitive>
Cel::Renderer::Asset::ProceduralLoader::create_cube(const glm::vec3 size)
{
    const Handle<Primitive> handle{ static_cast<uint32_t>(primitives.size()) };

    primitives.emplace_back();

    prepare({ .handle = handle, .shape = Cube{ .size = size } });

    return handle;
}

Cel::Renderer::Asset::Primitive&
Cel::Renderer::Asset::ProceduralLoader::get_primitive(Handle<Primitive> handle)
{
    return primitives[handle.index];
}

void
Cel::Renderer::Asset::ProceduralLoader::prepare(PrimitiveData data)
{
    std::visit(
        Overload{
            [&](const Cube& cube) { generate_cube(cube, data.primitive); },
            [](const Sphere& sphere) { throw_error(("unimplemented")); } },
        data.shape);

    std::unique_lock guard{ processingLock };
    processing.push_back(std::move(data));
}

void
Cel::Renderer::Asset::ProceduralLoader::generate_cube(const Cube& cube,
                                                      CpuPrimitive& primitive)
{
    // diagonal diagonal normal
    const glm::vec3 corners[] = {
        // Top face
        { -1, 1, -1 },
        { 1, 1, 1 },
        { 1, 1, -1 },
        { -1, 1, 1 },

        { 0, 1, 0 },
        // Left face
        { -1, 1, -1 },
        { -1, -1, 1 },
        { -1, 1, 1 },
        { -1, -1, -1 },

        { -1, 0, 0 },
        // Right face
        { 1, 1, 1 },
        { 1, -1, -1 },
        { 1, 1, -1 },
        { 1, -1, 1 },

        { 1, 0, 0 },
        // Bottom face
        { -1, -1, 1 },
        { 1, -1, -1 },
        { 1, -1, 1 },
        { -1, -1, -1 },

        { 0, -1, 0 },
        // Rear face
        { 1, 1, -1 },
        { -1, -1, -1 },
        { -1, 1, -1 },
        { 1, -1, -1 },

        { 0, 0, -1 },
        // Front face
        { -1, 1, 1 },
        { 1, -1, 1 },
        { 1, 1, 1 },
        { -1, -1, 1 },

        { 0, 0, 1 },
    };

    const glm::vec2 uvs[] = { { 0, 0 }, { 1, 1 }, { 1, 0 }, { 0, 1 } };

    // counterclockwise winding
    const uint32_t order[] = { 0, 3, 2, 1, 2, 3 };

    for (uint32_t i = 0; i < 30; i += 5) {
        for (const uint32_t corner : order) {
            primitive.vertices.push_back({ .position = corners[i + corner],
                                           .uvX = uvs[corner].x,
                                           .normal = corners[i + 4],
                                           .uvY = uvs[corner].y });
        }
    }
}

void
Cel::Renderer::Asset::ProceduralLoader::flush_cpu(SharedAssetData& shared)
{
    std::unique_lock guard{ processingLock };

    for (const auto& data : processing) {
        commit(data, shared);
    }

    processing.clear();
}

void
Cel::Renderer::Asset::ProceduralLoader::commit(const PrimitiveData& data,
                                               SharedAssetData& shared)
{
    const auto& [vertices, indices] = data.primitive;
    auto& primitive = primitives[data.handle.index];

    primitive.firstIndex =
        shared.indexBuffer.allocate(indices.data(),
                                    indices.size() * sizeof(uint32_t)) /
        sizeof(uint32_t);

    primitive.indexCount = indices.size();

    primitive.vertexOffset =
        shared.vertexBuffer.allocate(vertices.data(),
                                     vertices.size() * sizeof(Vertex)) /
        sizeof(Vertex);
}
