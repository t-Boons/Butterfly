#pragma once

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