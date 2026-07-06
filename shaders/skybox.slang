struct VSInput {
    float3 pos;
}

struct VSOutput {
    float4 pos : SV_Position;
    float3 uvw;
}

ConstantBuffer<float4x4> viewProj;

SamplerCube skybox;

[shader("vertex")]
VSOutput vertexMain(VSInput in) {
    VSOutput out;
    out.uvw = in.pos;

    float4 pos = mul(viewProj, float4(in.pos.xyz, 1.0));
    // Reverse Z buffer
    // Set skybox behind all
    pos = float4(pos.xy, 0.0, pos.w);
    out.pos = pos;

    return out;
}

[shader("fragment")]
float4 fragmentMain(VSOutput in) {
    return skybox.Sample(in.uvw);
}