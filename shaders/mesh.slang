#language slang 2026

import common;

struct VSInput
{
  uint drawId : SV_StartInstanceLocation;
  uint vertexId : SV_VulkanVertexID;
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
  let material = scene.materialBuffer[in.matIndex];

  return allTextures[material.colorTextureIndex].Sample(
      allSamplers[material.colorSamplerIndex], in.uv);
}
