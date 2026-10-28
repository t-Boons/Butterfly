#pragma once

#include "Common.hlsli"

float3 F_Schlick(float3 F0, float VoH)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
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