#pragma once

#define PI 3.14159265359
#define EPSILON 0.0001
#define FLT_MAX 3.402823466e+38F
#define MAX_CASCADES 4
#define CUBEMAP_SLICES 6

struct CameraData
{
    float4x4 ViewProj;
    float4x4 InvProj;
    float4x4 InvView;
    float3 Position;
    float Near;
    float Far;
};

struct ModelData
{
    float4x4 ModelMatrix;
    float4x4 InverseModelMatrix;
    float4x4 NormalMatrix;
    float4 SDFBoundsMin;
    float4 SDFBoundsSize;
    float4 SDFResolution;
    int SDFTextureIndex;
};

struct MaterialData
{
    float4 BaseColor;
    float4 EmissiveColor;
    float Metallic;
    float Roughness;
    float NormalScale;
    int ColorTexture;
    int NormalTexture;
    int MetallicRoughnessTexture;
    int EmissionTexture;
    int AmbientOcclusionTexture;
};


struct ShadowMap
{
    float4x4 LightViewProjections[CUBEMAP_SLICES];
    float CascadeSplits[MAX_CASCADES];
    float TexelSizes[MAX_CASCADES];
    int ShadowMapTextureIndex;
    int NumSlices;
    float Resolution;
};

struct Light
{
    uint Type;
    float3 Color;
    float3 Position;
    float3 Direction;
    float Range;
    float InnerConeAngleCos;
    float OuterConeAngleCos;
    ShadowMap Shadow;
};