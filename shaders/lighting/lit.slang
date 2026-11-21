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


for (uint i = 0 ; i < scene.lightBuffer.directionalLightCount)  {

let ptr = (DirectionalLight*)(uint64_t(scene.lightBuffer) + scene.lightBuffer.directionalLightOffset + i * sizeof(DirectionalLight));


}
}
