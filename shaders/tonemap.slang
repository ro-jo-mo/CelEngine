import common;

struct VSInput
{
  uint vertexId : SV_VulkanVertexID;
}

struct VSOutput
{
  float2 uv;
}

[shader("vertex")]
VSOutput
vertexMain(VSInput in)
{
  VSOutput out;

  let entityData = scene.entityDataBuffer[in.drawId];
  let vert = scene.vertexBuffer[in.vertexId];
}

[shader("fragment")]
float4
fragmentMain(VSOutput in)
{
}
