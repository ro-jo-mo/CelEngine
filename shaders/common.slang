#language slang 2026

module common;

public struct SceneData
{
  public Vertex* vertexBuffer;
  public MaterialData* materialBuffer;
  public PerEntityData* entityDataBuffer;
  public float4x4 view;
  public float4x4 projection;
  public float4x4 viewProj;
}

public struct Vertex
{
  public float3 position;
  public float uvx;
  public float3 normal;
  public float uvy;
}

public struct MaterialData
{
  public float4 colorFactors;
  public float4 metalRoughFactors;
  public uint colorTextureIndex;
  public uint metalRoughnessTextureIndex;
  public uint normalTextureIndex;
  public uint colorSamplerIndex;
  public uint metalRoughnessSamplerIndex;
  public uint normalSamplerIndex;
}

public struct PerEntityData
{
  public float4x4 transform;
  public float4x4 normalTransform;
  public uint32_t materialIndex;
}

[vk::binding(0, 0)]
public ConstantBuffer<SceneData> scene;


#ifndef MAX_SAMPLERS
#error "MAX_SAMPLERS value must be defined!"
#endif

[vk::binding(1, 0)]
public SamplerState allSamplers[MAX_SAMPLERS];

[vk::binding(2, 0)]
public Texture2D allTextures[];

// You can do this ?!
[vk::binding(2, 0)]
public TextureCube allCubeTextures[];

