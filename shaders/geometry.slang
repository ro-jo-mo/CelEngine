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

  out.position = mul(mul(scene.viewProj, entityData.transform), pos);
  out.normal = vert.normal;
  out.tangent = vert.tangent;
  out.uv = float2(vert.uvx, vert.uvy);
  out.matIndex = entityData.materialIndex;

  return out;
}

