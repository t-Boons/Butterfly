#pragma once

float SampleSDF(int sdf, int smp, float3 position, float3 boundsMin, float3 boundsSize)
{
    float3 boundsMax = boundsMin + boundsSize;
    float3 q = clamp(position, boundsMin, boundsMax);
    float outside = length(position - q);

    float3 uvw = (q - boundsMin) / boundsSize;
    Texture3D<float> sdfTexture = ResourceDescriptorHeap[sdf];
    SamplerState sdfSampler = SamplerDescriptorHeap[smp];
    float h = sdfTexture.SampleLevel(sdfSampler, uvw, 0);

    if (outside <= 0.0)
        return h;

    return sqrt(outside * outside + max(h, 0.0) * max(h, 0.0));
}

// Source: https://iquilezles.org/articles/rmshadows/
float SoftShadow(float3 origin, float3 dir, float3 boundsMin, float3 boundsSize, int sdf, int smp, float mint, float maxt, float w)
{
    float res = 1.0;
    float ph = 1e20;
    float t = mint;
    for (int i = 0; i < 256 && t < maxt; i++)
    {
        float3 p = origin + dir * t;
        float h = SampleSDF(sdf, smp, p, boundsMin, boundsSize);
        res = min(res, h / (w * t));
        t += clamp(h, 0.005, 0.50);
        if (res < -1.0 || t > maxt)
            break;
    }
    res = max(res, -1.0);
    return 0.25 * (1.0 + res) * (1.0 + res) * (2.0 - res);
}

float SoftShadowDirectional(float3 origin, float3 dir, float3 boundsMin, float3 boundsSize, int sdf, int smp, float mint, float maxt, float k)
{
    float res = 1.0;
    float t = mint;

    for (uint step = 0; step < 96 && t < maxt; ++step)
    {
        float3 p = origin + dir * t;
        float h = SampleSDF(sdf, smp, p, boundsMin, boundsSize);

        if (h < 0.001)
            return 0.0;

        res = min(res, k * h / max(t, 1e-4));

        t += h;
    }

    return saturate(res);
}

float ShadowPenumbraMargin(float3 origin, float3 boundsMin, float3 boundsSize, float w)
{
    float3 center = boundsMin + 0.5 * boundsSize;
    float maxT = length(center - origin) + 0.5 * length(boundsSize);
    return w * maxT;
}

float ShadowPenumbraMarginDirectional(float3 origin, float3 boundsMin, float3 boundsSize, float k)
{
    float3 center = boundsMin + 0.5 * boundsSize;
    float maxT = length(center - origin) + 0.5 * length(boundsSize);
    
    float margin = maxT / max(k, 1e-4);
    return margin;
}

bool IsInsideAABB(float3 position, float3 min, float3 max)
{
    return position.x >= min.x && position.x <= max.x &&
           position.y >= min.y && position.y <= max.y &&
           position.z >= min.z && position.z <= max.z;
}

float2 RayAABBIntersection(float3 rayOrigin, float3 rayDirection, float3 boundsMin, float3 boundsMax)
{
    float3 invDir = 1.0 / rayDirection;
    float3 tMin = (boundsMin - rayOrigin) * invDir;
    float3 tMax = (boundsMax - rayOrigin) * invDir;
    float tNear = max(max(min(tMin.x, tMax.x), min(tMin.y, tMax.y)), min(tMin.z, tMax.z));
    float tFar = min(min(max(tMin.x, tMax.x), max(tMin.y, tMax.y)), max(tMin.z, tMax.z));
    return float2(tNear, tFar);
}