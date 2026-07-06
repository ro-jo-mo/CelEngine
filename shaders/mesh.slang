struct SceneData
{
  float4x4 view;
  float4x4 projection;
  float4x4 viewProj;
}

struct GlobalData
{
  SceneData sceneData;
  Sampler2D allTextures[];
}

struct MaterialData
{
  float4 colorFactors;
  float4 metalRoughFactors;
  int colorTextureIndex;
  int metalRoughnessTextureIndex;
  int normalTextureIndex;
}

ParameterBlock<GlobalData> global;
ParameterBlock<MaterialData> material;

// Vertex shader

struct Vertex
{
  float3 position;
  float uvx;
  float3 normal;
  float uvy;
}

struct VSOutput
{
  float4 position : SV_Position;
  float3 normal;
  float2 uv;
}

[shader("vertex")]
VSOutput
vertexMain(uniform float4x4 transform,
           uniform Vertex* vertexBuffer,
           uint bufferIndex: SV_VertexID)
{
  VSOutput out;

  Vertex vert = vertexBuffer[bufferIndex];

  float4 pos = float4(vert.position, 1.0);

  out.position = mul(mul(global.sceneData.viewProj, transform), pos);
  out.normal = vert.normal;
  out.uv = float2(vert.uvx, vert.uvy);

  return out;
}

// Fragment shader

[shader("fragment")]
float4
fragmentMain(VSOutput in)
{
  Sampler2D colorTex = global.allTextures[material.colorTextureIndex];

  return colorTex.Sample(in.uv);
}
