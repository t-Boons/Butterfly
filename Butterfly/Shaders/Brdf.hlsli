#pragma once

#include "Common.hlsli"

float2 Hammersley(uint i, uint n)
{
    return float2(float(i) / float(n), reversebits(i) * 2.3283064365386963e-10f);
}

float3 ImportanceSampleGGX(float2 xi, float3 N, float alphaRoughness)
{
    float a = alphaRoughness;
    float phi = 2.0f * PI * xi.x;
    float cosTheta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    float3 H = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    float3 up = abs(N.z) < 0.999f ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);
    return normalize(right * H.x + up * H.y + N * H.z);
}

float3 F_Schlick(float3 F0, float VoH)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

float3 F_SchlickRoughness(float3 F0, float NoV, float roughness)
{
    return F0 + (max((1.0 - roughness).xxx, F0) - F0) * pow(saturate(1.0 - NoV), 5.0);
}

float D_GGX(float alphaRoughness, float NoH)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = NoH * NoH * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (PI * f * f);
}

float V_GGX(float alphaRoughness, float NoV, float NoL)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}