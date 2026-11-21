#language slang 2026

module common;

public struct SceneData
{
  public Vertex* vertexBuffer;
  public MaterialData* materialBuffer;
  public PerEntityData* entityDataBuffer;
  public LightData* lightBuffer;
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
  public float4 tangent;
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

public struct LightData
{
public    uint32_t directionalLightCount;
public    uint32_t pointLightCount;
public uint32_t directionalLightOffset;
public uint32_t pointLightOffset;
}

public struct PointLight
{
    public float3 colour;
    public float intensity;
    public float3 position;
}

public struct DirectionalLight
{
    public float3  direction;
     public float3 colour;
     public float intensity;
}

public struct GeometryOut
{
  public float4 position : SV_Position;
  public float3 normal;
  public float4 tangent;
  public float2 uv;
  public nointerpolation uint32_t matIndex;
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

