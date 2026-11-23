#language slang 2026

import common;

float3
get_normal(GeometryOut in, MaterialData material)
{
    // http://www.mikktspace.com/
    let tangent = in.tangent.xyz;
    let bitangent = in.tangent.w * cross(in.normal, tangent);

    // Sampled from normal map
    float3 tangentSpaceNormal =
        allTextures[material.normalTextureIndex]
            .Sample(allSamplers[material.normalSamplerIndex], in.uv)
            .xyz;

    tangentSpaceNormal = (2.0 * tangentSpaceNormal) - 1;

    return normalize(tangentSpaceNormal.x * tangent +
                     tangentSpaceNormal.y * bitangent +
                     tangentSpaceNormal.z * in.normal);
}

float3
light_calculation(GeometryOut in,
                  float3 normal,
                  float3 viewDir,
                  float3 lightDir,
                  float3 lightColour,
                  float3 materialColour,
                  float intensity,
                  float distance)
{
    // https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model

    let SPECULAR_COLOUR = float3(0.1, 0.1, 0.1);

    let lambertian = max(dot(lightDir, normal), 0.0);

    float3 diffuse = float3(0.0);
    float specular = 0.0;

    if (lambertian > 0.0) {
        let halfDir = normalize(lightDir + viewDir);
        let specAngle = max(dot(halfDir, normal), 0.0);
        specular = pow(specAngle, 4.0);
    }

    return (materialColour * lambertian * lightColour * intensity +
            SPECULAR_COLOUR * specular * lightColour) /
           distance;
}

float3
directional_light_calculation(GeometryOut in,
                              float3 normal,
                              float3 viewDir,
                              float3 materialColour,
                              DirectionalLight light)
{
    return light_calculation(in,
                             normal,
                             viewDir,
                             -light.direction,
                             light.colour,
                             materialColour,
                             light.intensity,
                             1.0);
}

float3
point_light_calculation(GeometryOut in,
                        float3 normal,
                        float3 viewDir,
                        float3 materialColour,
                        PointLight light)
{
    float3 lightDir = light.position - in.position;

    float distance = length(lightDir);

    lightDir /= distance;

    distance *= distance;

    return light_calculation(in,
                             normal,
                             viewDir,
                             lightDir,
                             light.colour,
                             materialColour,
                             light.intensity,
                             distance);
}

[shader("fragment")]
float4
fragment_shader(GeometryOut in)
{
    let material = scene.materialBuffer[in.matIndex];

    let normal = get_normal(in, material);

    let viewDir = normalize(scene.camera.position - in.position);

    let materialColour = allTextures[material.colorTextureIndex].Sample(
        allSamplers[material.colorSamplerIndex], in.uv);

    float3 out = float3(0.0);

    for (uint i = 0; i < scene.lightBuffer.directionalLightCount; i++) {
        let light =
            (DirectionalLight*)(uint64_t(scene.lightBuffer) +
                                scene.lightBuffer.directionalLightOffset +
                                i * sizeof(DirectionalLight));

        out += directional_light_calculation(
            in, normal, viewDir, materialColour.xyz, *light);
    }

    for (uint i = 0; i < scene.lightBuffer.pointLightCount; i++) {
        let light = (PointLight*)(uint64_t(scene.lightBuffer) +
                                  scene.lightBuffer.pointLightOffset +
                                  i * sizeof(PointLight));

        out += point_light_calculation(
            in, normal, viewDir, materialColour.xyz, *light);
    }

    return float4(out.xyz, 1.0);
}
