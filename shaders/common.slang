module common;

public struct SceneData
{
  public Vertex* vertexBuffer;
  public MaterialData* materialBuffer;
  public PerEntityData* entityDataBuffer;
  public float4x4 viewM;
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
  public int colorTextureIndex;
  public int metalRoughnessTextureIndex;
  public int normalTextureIndex;
}

public struct PerEntityData
{
  public float4x4 transform;
  public float4x4 normalTransform;
  public uint32_t materialIndex;
}

[vk::binding(0, 0)]
public ConstantBuffer<SceneData> scene;

[vk::binding(1, 0)]
public Sampler2D allTextures[];

