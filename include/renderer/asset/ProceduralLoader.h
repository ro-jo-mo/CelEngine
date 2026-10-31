#pragma once

#include "AssetTypes.h"
#include "common/Handle.h"

#include <glm/vec3.hpp>
#include <mutex>
#include <variant>
#include <vector>

namespace Cel::Renderer::Asset {

class ProceduralLoader
{
    struct PrimitiveData;

  public:
    Handle<Primitive> create_cube(glm::vec3 size = glm::vec3(1.0f));

    Primitive& get_primitive(Handle<Primitive> handle);

    void flush_cpu(SharedAssetData& shared);

  private:
    struct Cube
    {
        glm::vec3 size;
    };
    struct Sphere
    {
        float radius;
    };

    using Shape = std::variant<Cube, Sphere>;

    struct PrimitiveData
    {
        Handle<Primitive> handle;
        Shape shape;

        // Filled in by prepare
        CpuPrimitive primitive;
    };

    /**
     * Generates the geometry for the requested shape
     * The data created here is kept exclusively here until commit
     * Can be launched asynchronously
     * @param data
     */
    void prepare(PrimitiveData data);

    /**
     * Allocates the primitive in the shared buffers
     * Single threaded access
     */
    void commit(const PrimitiveData& data, SharedAssetData& shared);

    static void generate_cube(const Cube& cube, CpuPrimitive& primitive);

    std::mutex processingLock;
    std::vector<PrimitiveData> processing;

    std::vector<Primitive> primitives;
};

}
