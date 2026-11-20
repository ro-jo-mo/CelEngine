#language slang 2026

import common;

struct VSInput
{
  uint drawId : SV_StartInstanceLocation;
  uint vertexId : SV_VulkanVertexID;
}

[shader("fragment")]
float4
fragment_shader(GeometryOut in)
{
  let material = scene.materialBuffer[in.matIndex];

  return allTextures[material.colorTextureIndex].Sample(
    allSamplers[material.colorSamplerIndex], in.uv);
}
