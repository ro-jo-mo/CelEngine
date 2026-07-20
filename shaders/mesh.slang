import common;

struct VSInput
{
  uint drawId : SV_DrawIndex;
  uint vertexId : SV_VertexID;
}

struct VSOutput
{
  float4 position : SV_Position;
  float3 normal;
  float2 uv;
  nointerpolation uint32_t matIndex;
}

[shader("vertex")]
VSOutput
vertexMain(VSInput in)
{
  VSOutput out;

  let entityData = scene.entityDataBuffer[in.drawId];
  let vert = scene.vertexBuffer[in.vertexId];

  float4 pos = float4(vert.position, 1.0);

  out.position = mul(mul(scene.viewProj, entityData.transform), pos);
  out.normal = vert.normal;
  out.uv = float2(vert.uvx, vert.uvy);
  out.matIndex = entityData.materialIndex;

  return out;
}

[shader("fragment")]
float4
fragmentMain(VSOutput in)
{
  Sampler2D colorTex =
    allTextures[scene.materialBuffer[in.matIndex].colorTextureIndex];

  return colorTex.Sample(in.uv);
}
