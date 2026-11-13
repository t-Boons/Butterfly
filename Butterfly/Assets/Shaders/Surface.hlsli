#pragma once
#include "Common.hlsli"
#include "Material.hlsli"
#include "Brdf.hlsli"

struct Surface
{
    float4 Albedo;     
    float  Metallic;
    float  Roughness;  
    float3 Normal;          
    float3 ViewDir;       
    float3 WorldPos;       
    float3 F0;         
    float3 Emissive;
    float NoV;
    float AlphaRoughness;

    float NoL(float3 LightDir)          { return saturate(dot(Normal, LightDir)); }
    float3 HalfVector(float3 LightDir)  { return normalize(ViewDir + LightDir); }
    float NoH(float3 LightDir)          { return saturate(dot(Normal, HalfVector(LightDir))); }
    float VoH(float3 LightDir)          { return saturate(dot(ViewDir, HalfVector(LightDir))); }
    float3 Reflection()                 { return reflect(-ViewDir, Normal); }
};

inline Surface GetSurface(MaterialData material, float3 worldPos, float3 normal, float4 tangent, float2 uv, float3 cameraPosition)
{
    Surface surface;
    surface.Albedo = EvaluateAlbedo(material, uv);
    EvaluateMetallicRoughness(material, uv, surface.Metallic, surface.Roughness);
    surface.Normal = EvaluateNormal(material, normal, tangent, uv);
    surface.ViewDir = normalize(cameraPosition - worldPos);
    surface.F0 = lerp(float3(0.04, 0.04, 0.04), surface.Albedo.xyz, surface.Metallic);
    surface.WorldPos = worldPos;
    surface.Emissive = EvaluateEmissive(material, uv);
    surface.NoV = max(dot(surface.Normal, surface.ViewDir), 1e-4);
    surface.AlphaRoughness = surface.Roughness * surface.Roughness;
    return surface;
}

struct LightSample
{
    float3 L;
    float3 H;
    float NoL;
    float NoH;
    float VoH;
};

LightSample MakeLightSample(Surface s, float3 L)
{
    LightSample ls;
    ls.L = L;
    ls.H = normalize(s.ViewDir + L);
    ls.NoL = saturate(dot(s.Normal, L));
    ls.NoH = saturate(dot(s.Normal, ls.H));
    ls.VoH = saturate(dot(s.ViewDir, ls.H));
    return ls;
}

float3 EvaluateBRDF(Surface s, LightSample ls)
{
    if (ls.NoL <= 0.0) return 0.0;

    float3 F = F_Schlick(s.F0, ls.VoH);
    float D = D_GGX(s.AlphaRoughness, ls.NoH);
    float Vis = V_GGX(s.AlphaRoughness, s.NoV, ls.NoL);

    float3 specularBRDF = F * Vis * D;
    float3 diffuseBRDF = s.Albedo.rgb * (1.0 - s.Metallic) / PI;

    return (diffuseBRDF + specularBRDF) * ls.NoL;
}