#pragma once
#include "Common.hlsli"

#define POISSON_SAMPLE_COUNT 16 
#define PCSS_BLOCKER_SEARCH 0.075
#define PCSS_LIGHT_SIZE 0.06
#define PCSS_MIN_PENUMBRA_SIZE 0.00075

// Samples taken from:
// https://developer.download.nvidia.com/assets/gamedev/docs/PCSS_Integration.pdf 
static const float2 PCSS_Poisson_Samples[POISSON_SAMPLE_COUNT] =
{
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870),
    float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),
    float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),
    float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),
    float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),
    float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),
    float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),
    float2(0.14383161, -0.14100790)
};

/// Implementations based on this article:
/// https://gamedev.net/tutorials/programming/graphics/effect-area-light-shadows-part-1-pcss-r4971

inline float2 PCSS_Rotate(float2 uv, float2 seed)
{
    float angle = 6.2831853 * frac(52.9829189 * frac(dot(seed.xy, float2(0.06711056, 0.00583715))));
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);
}

// Texture2DArray Variants.

inline float PCF_Poisson(Texture2DArray<float> tex, int slice, SamplerComparisonState cmp, float3 projCoord, float radiusUV, float2 rotationSeed)
{
    float sum = 0.0;
    [Unroll]
    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Poisson_Samples[i] * radiusUV;
        offset = PCSS_Rotate(offset, rotationSeed);
        sum += tex.SampleCmpLevelZero(cmp, float3(projCoord.xy + offset, slice), projCoord.z);
    }
    return sum / POISSON_SAMPLE_COUNT;
}

inline float PCF_Grid(Texture2DArray<float> tex, int slice, SamplerComparisonState cmp, float3 projCoord, float radiusUV, int filterSize)
{
    float sum = 0.0;
    
    for (int i = -filterSize; i <= filterSize; i++)
    {
        for (int j = -filterSize; j <= filterSize; j++)
        {
            float2 offset = float2(i, j) * radiusUV;
            sum += tex.SampleCmpLevelZero(cmp, float3(projCoord.xy + offset, slice), projCoord.z);
        }
    }
    return sum / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
}

inline float PCSS_BlockerDistance(Texture2DArray<float> tex, int slice, SamplerState smp, float3 projCoord, float searchUV, float2 rotationSeed)
{
    int blockers = 0;
    float avgBlocker = 0.0;
    float2 newSeed = rotationSeed + float2(37.0, 17.0);
    
    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Poisson_Samples[i] * searchUV;
        offset = PCSS_Rotate(offset, newSeed);
        
        float z = tex.SampleLevel(smp, float3(projCoord.xy + offset, float(slice)), 0);
        if (z < projCoord.z)
        {
            blockers++;
            avgBlocker += z;
        }
    }

    if (blockers == 0)
        return 0.0;
    
    avgBlocker /= blockers;
    
    return avgBlocker;
}


inline float PCSS_Shadow(Texture2DArray<float> map, int slice, SamplerState blockerSampler, SamplerComparisonState pcssSampler, float3 projCoords, float lightSize, float2 rotationSeed)
{
    float averageBlockerDistance = PCSS_BlockerDistance(map, slice, blockerSampler, projCoords, PCSS_BLOCKER_SEARCH, rotationSeed);

	if (averageBlockerDistance == 0.0)
	{
		return 1.0;
	}
	else
	{
        float penumbraSize = lightSize * (projCoords.z - averageBlockerDistance);
        penumbraSize = max(penumbraSize, PCSS_MIN_PENUMBRA_SIZE);
        float shadow = PCF_Poisson(map, slice, pcssSampler, projCoords, penumbraSize, rotationSeed);
        return shadow;
    }
}

inline float PCSS_CSM_Shadow(float3 worldPosition, float3 worldNormal, float3 lightDirection, float cameraDepth, float cascadeSplits[MAX_CASCADES], float cascadeTexelSizes[MAX_CASCADES], float4x4 LightViewProjections[CUBEMAP_SLICES], Texture2DArray<float> shadowMaps, SamplerState blockerSampler, SamplerComparisonState pcssSmp, float resolution, float2 rotationSeed)
{
    uint cascadeIndex = 0;
    for (uint i = 0; i < MAX_CASCADES; ++i)
    {
        if (cameraDepth <= cascadeSplits[i])
        {
            cascadeIndex = i;
            break;
        }
        cascadeIndex = i;
    }
    float NoL = saturate(dot(normalize(worldNormal), normalize(lightDirection)));

    float4x4 lightVP = LightViewProjections[cascadeIndex];
    float4 lightClip = mul(lightVP, float4(worldPosition, 1.0));
    float3 lightNDC = lightClip.xyz / lightClip.w;
    float2 uv = float2(lightNDC.x * 0.5 + 0.5, -lightNDC.y * 0.5 + 0.5);
    
    // Early out if the fragment is outside the shadow map bounds
    if (any(uv < 0.0) || any(uv > 1.0) || lightNDC.z < 0.0 || lightNDC.z > 1.0)
    {
        return 1.0;
    }
    
    float3 projectedPos = float3(uv, lightNDC.z - 0.008);

    return PCSS_Shadow(shadowMaps, cascadeIndex, blockerSampler, pcssSmp, projectedPos, PCSS_LIGHT_SIZE, rotationSeed);
}


// Texture2D Variants.

inline float PCF_Poisson(Texture2D<float> tex, SamplerComparisonState cmp, float3 projCoord, float radiusUV, float2 rotationSeed)
{
    float sum = 0.0;
    [unroll]
    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Rotate(PCSS_Poisson_Samples[i] * radiusUV, rotationSeed);
        sum += tex.SampleCmpLevelZero(cmp, projCoord.xy + offset, projCoord.z);
    }
    return sum / POISSON_SAMPLE_COUNT;
}

inline float PCF_Grid(Texture2D<float> tex, SamplerComparisonState cmp, float3 projCoord, float radiusUV, int filterSize)
{
    float sum = 0.0;
    for (int i = -filterSize; i <= filterSize; i++)
        for (int j = -filterSize; j <= filterSize; j++)
            sum += tex.SampleCmpLevelZero(cmp, projCoord.xy + float2(i, j) * radiusUV, projCoord.z);

    return sum / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
}

inline float PCSS_BlockerDistance(Texture2D<float> tex, SamplerState smp, float3 projCoord, float searchUV, float2 rotationSeed)
{
    int blockers = 0;
    float avgBlocker = 0.0;
    float2 newSeed = rotationSeed + float2(37.0, 17.0);

    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Rotate(PCSS_Poisson_Samples[i] * searchUV, newSeed);
        float z = tex.SampleLevel(smp, projCoord.xy + offset, 0);
        if (z < projCoord.z)
        {
            blockers++;
            avgBlocker += z;
        }
    }

    return blockers == 0 ? 0.0 : avgBlocker / blockers;
}

inline float PCSS_Shadow(Texture2D<float> map, SamplerState blockerSampler, SamplerComparisonState pcssSampler, float3 projCoords, float lightSize, float2 rotationSeed)
{
    float blocker = PCSS_BlockerDistance(map, blockerSampler, projCoords, PCSS_BLOCKER_SEARCH, rotationSeed);
    if (blocker == 0.0)
        return 1.0;

    float penumbra = max(lightSize * (projCoords.z - blocker), PCSS_MIN_PENUMBRA_SIZE);
    return PCF_Poisson(map, pcssSampler, projCoords, penumbra, rotationSeed);
}


// TextureCube variants

inline void CubeBasis(float3 dir, out float3 t, out float3 b)
{
    float3 up = abs(dir.y) < 0.99 ? float3(0, 1, 0) : float3(1, 0, 0);
    t = normalize(cross(up, dir));
    b = cross(dir, t);
}

inline float3 CubeOffset(float3 dir, float3 t, float3 b, float2 offsetUV)
{
    float3 a = abs(dir);
    float major = max(a.x, max(a.y, a.z));
    return dir + (t * offsetUV.x + b * offsetUV.y) * major * 2.0;
}

inline float PCF_Poisson(TextureCube<float> tex, float3 dir, SamplerComparisonState cmp, float depth, float radiusUV, float2 rotationSeed)
{
    float3 t, b;
    CubeBasis(dir, t, b);

    float sum = 0.0;
    [unroll]
    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Rotate(PCSS_Poisson_Samples[i] * radiusUV, rotationSeed);
        sum += tex.SampleCmpLevelZero(cmp, CubeOffset(dir, t, b, offset), depth);
    }
    return sum / POISSON_SAMPLE_COUNT;
}

inline float PCF_Grid(TextureCube<float> tex, float3 dir, SamplerComparisonState cmp, float depth, float radiusUV, int filterSize)
{
    float3 t, b;
    CubeBasis(dir, t, b);

    float sum = 0.0;
    for (int i = -filterSize; i <= filterSize; i++)
        for (int j = -filterSize; j <= filterSize; j++)
            sum += tex.SampleCmpLevelZero(cmp, CubeOffset(dir, t, b, float2(i, j) * radiusUV), depth);

    return sum / ((filterSize * 2 + 1) * (filterSize * 2 + 1));
}

inline float PCSS_BlockerDistance(TextureCube<float> tex, float3 dir, SamplerState smp, float depth, float searchUV, float2 rotationSeed)
{
    float3 t, b;
    CubeBasis(dir, t, b);

    int blockers = 0;
    float avgBlocker = 0.0;
    float2 newSeed = rotationSeed + float2(37.0, 17.0);

    for (int i = 0; i < POISSON_SAMPLE_COUNT; i++)
    {
        float2 offset = PCSS_Rotate(PCSS_Poisson_Samples[i] * searchUV, newSeed);
        float z = tex.SampleLevel(smp, CubeOffset(dir, t, b, offset), 0);
        if (z < depth)
        {
            blockers++;
            avgBlocker += z;
        }
    }

    return blockers == 0 ? 0.0 : avgBlocker / blockers;
}

inline float PCSS_Shadow(TextureCube<float> map, float3 dir, SamplerState blockerSampler, SamplerComparisonState pcssSampler, float depth, float lightSize, float2 rotationSeed)
{
    float blocker = PCSS_BlockerDistance(map, dir, blockerSampler, depth, PCSS_BLOCKER_SEARCH, rotationSeed);
    if (blocker == 0.0)
        return 1.0;

    float penumbra = max(lightSize * (depth - blocker), PCSS_MIN_PENUMBRA_SIZE);
    return PCF_Poisson(map, dir, pcssSampler, depth, penumbra, rotationSeed);
}