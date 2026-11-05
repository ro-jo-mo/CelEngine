#language slang 2026

import common;

struct VSOutput {
    float4 pos : SV_Position;
    float2 uv;
}


[shader("vertex")]
VSOutput
vertexMain(uint vertexId : SV_VulkanVertexID)
{
  VSOutput out;

  // Fullscreen triangle
  out.uv = float2(((vertexId & 1) << 2),vertexId & 2 );
  out.pos = float4(out.uv * 2.0 - 1.0, 0.0, 1.0);

  return out;
}


// Source from https://www.shadertoy.com/view/mdcSDH

float3 agx_default_contrast_approx(float3 x) {
  float3 x2 = x * x;
  float3 x4 = x2 * x2;
  
  return + 15.5     * x4 * x2
         - 40.14    * x4 * x
         + 31.96    * x4
         - 6.868    * x2 * x
         + 0.4298   * x2
         + 0.1191   * x
         - 0.00232;
}

float3 agx(float3 val) {
  const float3x3 agx_mat = float3x3(
    0.842479062253094, 0.0423282422610123, 0.0423756549057051,
    0.0784335999999992,  0.878468636469772,  0.0784336,
    0.0792237451477643, 0.0791661274605434, 0.879142973793104);
  const float minEv = -12.47393f;
  const float maxEv = 4.026069f;

  // Input transform
  val = mul(val, agx_mat);
  
  // Log2 space encoding
  val = clamp(log2(max(val,0.0000000001)), minEv, maxEv);
  val = (val - minEv) / (maxEv - minEv);
  
  // Apply sigmoid function approximation
  val = agx_default_contrast_approx(val);

  return val;
}

float3 agx_eotf(float3 val) {
  const float3x3 agx_mat_inv = float3x3(
    1.19687900512017, -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368, 1.15190312990417, -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433, 1.15107367264116);

  return mul(val, agx_mat_inv);
}


[shader("fragment")]
float4
fragmentMain(VSOutput in, uniform uint imageId)
{
  float4 colour = allTextures[imageId].Load(int3(int2(in.pos.xy),0));

  float3 display = saturate(agx_eotf(agx(colour.xyz)));

  return float4(display, 1.0);
}
