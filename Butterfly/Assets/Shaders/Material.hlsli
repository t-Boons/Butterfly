#pragma once
#include "Common.hlsli"
#include "Normal.hlsli"

struct MaterialData
{
    float4 BaseColor;
    float4 EmissiveColor;
    float Metallic;
    float Roughness;
    float NormalScale;
    int ColorTexture;
    int ColorTextureSampler;
    int NormalTexture;
    int NormalTextureSampler;
    int MetallicRoughnessTexture;
    int MetallicRoughnessTextureSampler;
    int EmissionTexture;
    int EmissionTextureSampler;
    int AmbientOcclusionTexture;
    int AmbientOcclusionTextureSampler;
};


inline float3 EvaluateNormal(MaterialData material, float3 inputNormal, float4 inputTangent, float2 uv)
{
    float3 normal = normalize(inputNormal);
    if (IsValid(material.NormalTexture))
    {
        Texture2D<float4> normalTex = ResourceDescriptorHeap[material.NormalTexture];
        SamplerState smp = SamplerDescriptorHeap[material.NormalTextureSampler];
        float3 sampledNormal = normalTex.Sample(smp, uv).xyz;
        normal = MapNormal(sampledNormal, normal, inputTangent.xyz, inputTangent.w, material.NormalScale);
    }
    return normal;
}

inline float4 EvaluateAlbedo(MaterialData material, float2 uv)
{
    float4 albedo = material.BaseColor;
    if (IsValid(material.ColorTexture))
    {
        Texture2D<float4> albedoTex = ResourceDescriptorHeap[material.ColorTexture];
        SamplerState smp = SamplerDescriptorHeap[material.ColorTextureSampler];
        albedo *= albedoTex.Sample(smp, uv);
    }
    return albedo;
}

inline void EvaluateMetallicRoughness(MaterialData material, float2 uv, out float metallic, out float perceptualRoughness)
{
    metallic = material.Metallic;
    perceptualRoughness = material.Roughness;
    if (IsValid(material.MetallicRoughnessTexture))
    {
        Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[material.MetallicRoughnessTexture];
        SamplerState smp = SamplerDescriptorHeap[material.MetallicRoughnessTextureSampler];
        float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(smp, uv);
        metallic = metallicRoughnessSample.z;
        perceptualRoughness = metallicRoughnessSample.y;
    }
    metallic = saturate(metallic);
    perceptualRoughness = max(perceptualRoughness, 0.045);
}

inline float3 EvaluateEmissive(MaterialData material, float2 uv)
{
    float3 emissive = material.EmissiveColor.xyz;
    if (IsValid(material.EmissionTexture))
    {
        Texture2D<float4> emissiveTex = ResourceDescriptorHeap[material.EmissionTexture];
        SamplerState smp = SamplerDescriptorHeap[material.EmissionTextureSampler];
        emissive = emissiveTex.Sample(smp, uv).xyz;
    }
    return emissive;
}

inline float EvaluateAO(MaterialData material, float2 uv)
{
    if (!IsValid(material.AmbientOcclusionTexture)) return 1.0;
    Texture2D<float4> aoTex = ResourceDescriptorHeap[material.AmbientOcclusionTexture];
    SamplerState smp = SamplerDescriptorHeap[material.AmbientOcclusionTextureSampler];
    return aoTex.Sample(smp, uv).x;
}
