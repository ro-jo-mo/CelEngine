#language slang 2026

import common;

struct VSInput
{
    uint drawId : SV_StartInstanceLocation;
    uint vertexId : SV_VulkanVertexID;
}

[shader("vertex")]
GeometryOut
vertex_shader(VSInput in)
{
    GeometryOut out;

    let entityData = scene.entityDataBuffer[in.drawId];
    let vert = scene.vertexBuffer[in.vertexId];

    float4 pos = float4(vert.position, 1.0);

    out.clipSpaceCoord =
        mul(mul(scene.camera.viewProj, entityData.transform), pos);

    out.position = mul(entityData.transform, pos).xyz;

    out.uv = float2(vert.uvx, vert.uvy);
    out.matIndex = entityData.materialIndex;

    let normalTrans = float3x3(entityData.normalTransform);
    let trans = float3x3(entityData.transform);

    out.normal = normalize(mul(normalTrans, vert.normal));
    out.tangent =
        float4(normalize(mul(trans, vert.tangent.xyz)), vert.tangent.w);

    return out;
}
