#language slang 2026

import common;


struct VSInput {
    uint vertexId : SV_VertexID;
}


struct VSOutput {
    float4 pos : SV_Position;
    float3 uvw;
    uint imageId; 
}


[shader("vertex")]
VSOutput vertexMain(VSInput in, uniform uint imageId) {
    VSOutput out;

    out.imageId = imageId;

    float3 pos = scene.vertexBuffer[in.vertexId].position;

    out.uvw = pos;

    out.pos = mul(scene.viewProj, float4(pos.xyz, 1.0));
    // Reverse Z buffer
    // Set skybox behind all
    out.pos = float4(out.pos.xy, 0.0, out.pos.w);
    
    return out;
}

[shader("fragment")]
float4 fragmentMain(VSOutput in) {
    return allCubeTextures[in.imageId].Sample(in.uvw);
}