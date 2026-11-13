#include "Brdf.hlsli"
#include "Sdf.hlsli"
#include "Shadow.hlsli"
#include "Normal.hlsli"
#include "Surface.hlsli"
#include "FrameData.hlsli"
#include "Camera.hlsli"
#include "Model.hlsli"
#include "Light.hlsli"

struct BufferIndices
{
    int frameDataIndex;

    int meshIndex;
    int modelIndex;
    int materialIndex;

    int skyboxTextureIndex;

    int sdfSamplerIndex;
    int blockerSamplerIndex;
    int pcssShadowSamplerIndex;

    SamplerComparisonState GetPCSSShadowSmp()
    {
        return SamplerDescriptorHeap[pcssShadowSamplerIndex];
    }
    SamplerState GetBlockerSmp()
    {
        return SamplerDescriptorHeap[blockerSamplerIndex];
    }
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

float3 EvaluteLight(Surface surface, Light light, float worldDepth, FrameData frameData, int i)
{
    if (light.Type == 0) // Directional Light
    {
        LightSample ls = MakeLightSample(surface, normalize(-light.Direction));

        float3 brdf = EvaluateBRDF(surface, ls);
        if (ls.NoL <= 0.0)
        {
            return float3(0.0, 0.0, 0.0);
        }


        float shadow = 1.0;

#ifdef USE_PCSS_SHADOWS
        ShadowMap shadowMap = frameData.GetShadowMap(i);
        if (shadowMap.HasShadowMap())
        {
            Texture2DArray<float> shadowMapTexture = shadowMap.ShadowMapTextureArray();

            uint cascadeIndex = 0;
            for (uint i = 0; i < MAX_CASCADES; ++i)
            {
                if (worldDepth <= shadowMap.CascadeSplits[i])
                {
                    cascadeIndex = i;
                    break;
                }
                cascadeIndex = i;
            }

            float4x4 lightVP = shadowMap.LightViewProjections[cascadeIndex];
            float4 lightClip = mul(lightVP, float4(surface.WorldPos, 1.0));
            float3 lightNDC = lightClip.xyz / lightClip.w;
            float2 uv = float2(lightNDC.x * 0.5 + 0.5, -lightNDC.y * 0.5 + 0.5);

            if (!(any(uv < 0.0) || any(uv > 1.0) || lightNDC.z < 0.0 || lightNDC.z > 1.0))
            {
                float bias = 0.0005f + 0.01f * (1.0f - ls.NoL);
                float3 projectedPos = float3(uv, lightNDC.z - bias);

                shadow = PCSS_Shadow(shadowMapTexture, cascadeIndex, resources.GetBlockerSmp(), resources.GetPCSSShadowSmp(), projectedPos, PCSS_LIGHT_SIZE, surface.WorldPos.xy);
            }
        }
#endif

        return brdf * light.Color * shadow;
    }

    float distance = length(light.Position - surface.WorldPos);
    if (distance >= light.Range)
    {
        return float3(0.0, 0.0, 0.0);
    }

    float3 lightDir = normalize(light.Position - surface.WorldPos);
    float inverseSquare = 1.0 / max(distance * distance, EPSILON);
    float rangeFade = 1.0 - saturate(distance / light.Range);
    rangeFade *= rangeFade;
    float distanceAttenuation = inverseSquare * rangeFade;

    if (light.Type == 1) // Point Light
    {
        if (distanceAttenuation <= 0.0)
        {
            return float3(0.0, 0.0, 0.0);
        }

        LightSample ls = MakeLightSample(surface, lightDir);

        float3 brdf = EvaluateBRDF(surface, ls);
        if (ls.NoL <= 0.0)
        {
            return float3(0.0, 0.0, 0.0);
        }


        float shadow = 1.0f;
#ifdef USE_PCSS_SHADOWS
        ShadowMap shadowMap = frameData.GetShadowMap(i);
        if (shadowMap.HasShadowMap())
        {
            TextureCube<float> shadowMapTexture = shadowMap.ShadowMapTextureCube();

            float3 direction = surface.WorldPos - light.Position;
            float3 absDirection = abs(direction);
            uint faceIndex;
            if (absDirection.x >= absDirection.y && absDirection.x >= absDirection.z)
                faceIndex = direction.x >= 0.0f ? 0 : 1;
            else if (absDirection.y >= absDirection.x && absDirection.y >= absDirection.z)
                faceIndex = direction.y >= 0.0f ? 2 : 3;
            else
                faceIndex = direction.z >= 0.0f ? 4 : 5;

            float bias = 0.0008f + 0.01f * (1.0f - ls.NoL);
            float4 lightClip = mul(shadowMap.LightViewProjections[faceIndex], float4(surface.WorldPos, 1.0));
            float receiverDepth = lightClip.z / lightClip.w - bias;

            shadow = PCSS_Shadow(shadowMapTexture, direction, resources.GetBlockerSmp(), resources.GetPCSSShadowSmp(), receiverDepth, PCSS_LIGHT_SIZE, surface.WorldPos.xy);

            if (shadow <= 0.0)
            {
                return float3(0.0, 0.0, 0.0);
            }
        }
#endif

        return brdf * distanceAttenuation * light.Color * shadow;
    }

    if (light.Type == 2) // Spot light
    {
        float cone = smoothstep(light.OuterConeAngleCos, light.InnerConeAngleCos, dot(-lightDir, normalize(light.Direction)));
        float attenuation = distanceAttenuation * cone;

        if (attenuation <= 0.0)
        {
            return float3(0.0, 0.0, 0.0);
        }

        LightSample ls = MakeLightSample(surface, lightDir);

        float3 brdf = EvaluateBRDF(surface, ls);
        if (ls.NoL <= 0.0)
        {
            return float3(0.0, 0.0, 0.0);
        }


        float shadow = 1.0f;

#ifdef USE_PCSS_SHADOWS
        ShadowMap shadowMap = frameData.GetShadowMap(i);
        if (shadowMap.HasShadowMap())
        {
            Texture2D<float> shadowMapTexture = shadowMap.ShadowMapTexture2D();
            float bias = 0.0008f + 0.01f * (1.0f - ls.NoL);

            float4x4 lightVP = shadowMap.LightViewProjections[0];
            float4 lightClip = mul(lightVP, float4(surface.WorldPos, 1.0));
            float3 lightNDC = lightClip.xyz / lightClip.w;
            float2 uv = float2(lightNDC.x * 0.5 + 0.5, -lightNDC.y * 0.5 + 0.5);

            if (!(any(uv < 0.0) || any(uv > 1.0) || lightNDC.z < 0.0 || lightNDC.z > 1.0))
            {
                float3 projectedPos = float3(uv, lightNDC.z - bias);

                shadow = PCSS_Shadow(shadowMapTexture, resources.GetBlockerSmp(), resources.GetPCSSShadowSmp(), projectedPos, PCSS_LIGHT_SIZE, surface.WorldPos.xy);

                if (shadow <= 0.0)
                {
                    return float3(0.0, 0.0, 0.0);
                }
            }
        }
#endif

        return brdf * attenuation * light.Color * shadow;
    }

    return float3(0.0, 0.0, 0.0);
}

float4 main(V2P pixelInput) : SV_TARGET0
{
    ConstantBuffer<FrameData> frameData = ResourceDescriptorHeap[resources.frameDataIndex];

    MaterialData material = frameData.GetMaterial(resources.materialIndex);
    CameraData uniforms = frameData.GetCameraData();

    Surface surface = GetSurface(material, pixelInput.fragPos, pixelInput.normal, pixelInput.tangent, pixelInput.texCoord, uniforms.Position);

    float3 ambient = float3(0.05, 0.05, 0.07);

    float3 lighting = ambient * (surface.Albedo.rgb * (1.0 - surface.Metallic));

    SamplerComparisonState pcssShadowSmp = SamplerDescriptorHeap[resources.pcssShadowSamplerIndex];
    SamplerState blockerSmp = SamplerDescriptorHeap[resources.blockerSamplerIndex];
    float worldDepth = pixelInput.position.w;

    for (int i = 0; i < frameData.GetNumLights(); i++)
    {
        Light light = frameData.GetLight(i);
        lighting += EvaluteLight(surface, light, worldDepth, frameData, i);
    }

    float3 fragColor = lighting.xyz + surface.Emissive * material.EmissiveColor.rgb;

#if USE_SDF_SHADOWS
    SamplerState sdfSampler = SamplerDescriptorHeap[resources.sdfSamplerIndex];
    StructuredBuffer<ModelData> modelData = ResourceDescriptorHeap[resources.modelIndex];

    float shadow = 1.0;
    float k = 32.0;
    for (int i = 0; i < resources.numLights; i++)
    {
        if (lights[i].Type == 0)
        {
            Light light = lights[i];

            float NoL = saturate(dot(normalize(pixelInput.normal), -light.Direction));
            for (int m = 0; m < resources.numModels; m++)
            {
                ModelData model = modelData[m];
                if (model.SDFTextureIndex < 0)
                    continue;

                float3 boundMin = model.SDFBoundsMin.xyz;
                float3 boundSize = model.SDFBoundsSize.xyz;

                float3 originW = pixelInput.fragPos + pixelInput.normal * EPSILON;
                float3 originM = mul(model.InverseModelMatrix, float4(originW, 1.0)).xyz;
                float3 dirM = normalize(mul(model.InverseModelMatrix, float4(-light.Direction, 0.0)).xyz);

                float margin = ShadowPenumbraMarginDirectional(originM, boundMin, boundSize, k);
                float2 tNF = RayAABBIntersection(originM, dirM, boundMin - margin, boundMin + boundSize + margin);

                bool isSelf = (m == resources.entityIndex);

                float fragmentDistanceToModel = SampleSDF(model.SDFTextureIndex, resources.sdfSamplerIndex, originM, boundMin, boundSize);

                bool startsInContact = isSelf || fragmentDistanceToModel < 0.05;
                float contactSkip = startsInContact ? 0.05 / max(NoL, 0.1) : 0.0;

                float minT = max(tNF.x, contactSkip);
                float maxT = tNF.y;

                if (minT >= maxT)
                    continue;

                float s = SoftShadowDirectional(originM, dirM, boundMin, boundSize, model.SDFTextureIndex, resources.sdfSamplerIndex, minT, maxT, k);

                if (isSelf)
                    s = lerp(1.0, s, smoothstep(0.0, 0.25, NoL));

                shadow *= s;
                if (shadow <= EPSILON)
                    break;
            }
        }

        fragColor = lerp(ambient * diffuseColor, fragColor, shadow);
    }
#endif

    return float4(fragColor, surface.Albedo.w);
}