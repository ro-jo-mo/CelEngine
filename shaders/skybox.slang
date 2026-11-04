#language slang 2026

import common;


struct VSInput {
    uint vertexId : SV_VulkanVertexID;
}


struct VSOutput {
    float4 pos : SV_Position;
    float3 uvw; 
}


[shader("vertex")]
VSOutput vertexMain(VSInput in) {
    VSOutput out;

    float3 pos = scene.vertexBuffer[in.vertexId].position;

    out.uvw = pos;

    // Remove the translation from the transform
    float3 viewDir = mul(scene.view, float4(pos, 0.0)).xyz;

    out.pos = mul(scene.projection, float4(viewDir, 1.0));

    // Reverse Z buffer
    // Set skybox behind all
    out.pos = float4(out.pos.xy, 0.0, out.pos.w);
    
    return out;
}

struct SkyboxPush {
    uint imageId;
    uint samplerId;
}

[shader("fragment")]
float4 fragmentMain(VSOutput in, uniform SkyboxPush push) {
    return allCubeTextures[push.imageId].Sample(allSamplers[push.samplerId], in.uvw);
}