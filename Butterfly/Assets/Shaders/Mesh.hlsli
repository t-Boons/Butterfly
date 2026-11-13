#pragma once
#include "Common.hlsli"

struct Mesh
{
    int Positions;
    int Normals;
    int Tangents;
    int UVs;

    bool HasNormals()  { return IsValid(Normals); }
    bool HasTangents() { return IsValid(Tangents); }
    bool HasUVs()      { return IsValid(UVs); }

    inline float3 GetPosition(uint v)
    {
        StructuredBuffer<float3> b = ResourceDescriptorHeap[Positions];
        return b[v];
    }

    inline float3 GetNormal(uint v)
    {
        if (!HasNormals()) return float3(0, 0, 1);
        StructuredBuffer<float3> b = ResourceDescriptorHeap[Normals];
        return b[v];
    }

    inline float4 GetTangent(uint v)
    {
        if (!HasTangents()) return float4(1, 0, 0, 1);
        StructuredBuffer<float4> b = ResourceDescriptorHeap[Tangents];
        return b[v];
    }

    inline float2 GetUV(uint v)
    {
        if (!HasUVs()) return float2(0, 0);
        StructuredBuffer<float2> b = ResourceDescriptorHeap[UVs];
        return b[v];
    }
};