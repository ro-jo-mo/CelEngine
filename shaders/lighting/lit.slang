#language slang 2026

import common;

struct VSInput
{
    uint drawId : SV_StartInstanceLocation;
    uint vertexId : SV_VulkanVertexID;
}

float3
get_normal(GeometryOut in, MaterialData material)
{
    // http://www.mikktspace.com/
    let tangent = in.tangent.xyz;
    let bitangent = in.tangent.w * cross(in.normal, tangent);

    // Sampled from normal map
    float3 tangentSpaceNormal =
        allTextures[material.normalTextureIndex]
            .Sample(allSamplers[material.normalSamplerIndex], in.uv)
            .xyz;

    tangentSpaceNormal = (2.0 * tangentSpaceNormal) - 1;

    return normalize(tangentSpaceNormal.x * tangent +
                     tangentSpaceNormal.y * bitangent +
                     tangentSpaceNormal.z * in.normal);
}

[shader("fragment")]
float4
fragment_shader(GeometryOut in)
{
    let material = scene.materialBuffer[in.matIndex];

    for (uint i = 0; i < scene.lightBuffer.directionalLightCount; i++) {

        let ptr = (DirectionalLight*)(uint64_t(scene.lightBuffer) +
                                      scene.lightBuffer.directionalLightOffset +
                                      i * sizeof(DirectionalLight));
    }

    for (uint i = 0; i < scene.lightBuffer.pointLightCount; i++) {

        let ptr = (PointLight*)(uint64_t(scene.lightBuffer) +
                                scene.lightBuffer.pointLightOffset +
                                i * sizeof(PointLight));
    }

    return allTextures[material.colorTextureIndex].Sample(
        allSamplers[material.colorSamplerIndex], in.uv);
}
