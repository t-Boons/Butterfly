#define NUM_CASCADES 4

#define BLOCKER_SEARCH_SAMPLES 16
#define PCF_SAMPLES 32

#define LIGHT_SIZE_UV 0.04
#define MAX_FILTER_RADIUS_UV 0.01

static const float2 PoissonDisk[32] =
{
    float2(-0.613, 0.617), float2(0.170, -0.040),
    float2(-0.299, -0.791), float2(0.645, 0.493),
    float2(-0.651, -0.280), float2(0.421, -0.714),
    float2(0.887, -0.155), float2(-0.105, 0.263),
    float2(-0.914, 0.163), float2(0.284, 0.890),
    float2(-0.388, 0.425), float2(0.697, -0.446),
    float2(-0.024, -0.573), float2(0.515, 0.130),
    float2(-0.746, -0.633), float2(0.057, 0.719),
    float2(-0.474, -0.091), float2(0.370, 0.536),
    float2(-0.182, -0.940), float2(0.940, 0.329),
    float2(-0.833, 0.500), float2(0.230, -0.420),
    float2(-0.530, 0.790), float2(0.770, -0.690),
    float2(-0.120, 0.960), float2(0.040, -0.180),
    float2(-0.960, -0.360), float2(0.600, 0.750),
    float2(-0.340, -0.490), float2(0.480, -0.020),
    float2(0.800, 0.100), float2(-0.700, 0.050)
};

struct GPUShadowMap
{
    float4x4 LightViewProjections[NUM_CASCADES];
    float CascadeSplits[NUM_CASCADES];
    int ShadowMapTextureIndex;
    int NumCascades;
    float Size;
};


float CalculateShadow(float3 worldPosition, float4x4 lightViewProjection, Texture2DArray<float> shadowMapTexture, int slice, SamplerState blockerSampler, SamplerComparisonState pcssSampler, float NoL, float shadowMapSize)
{
    float4 lightClip = mul(lightViewProjection, float4(worldPosition, 1.0));
    float3 lightNDC = lightClip.xyz / lightClip.w;

    float2 uv = float2(lightNDC.x * 0.5 + 0.5, -lightNDC.y * 0.5 + 0.5);

    float receiverDepth = lightNDC.z;

    if (any(uv < 0.0) || any(uv > 1.0) || receiverDepth < 0.0 || receiverDepth > 1.0) return 1.0;

    float bias = lerp(0.005, 0.03, 1.0 - NoL);
    float compareDepth = receiverDepth - bias;
    float3 uvw = float3(uv, float(slice));

    // 1. Blocker search: find shadow-map depths closer to the light.
    float blockerDepthSum = 0.0;
    float blockerCount = 0.0;

    float searchRadius = 4.0 / shadowMapSize;

    [unroll]
    for (int i = 0; i < BLOCKER_SEARCH_SAMPLES; ++i)
    {
        float2 sampleUV = uv + PoissonDisk[i] * searchRadius;

        if (any(sampleUV < 0.0) || any(sampleUV > 1.0))
            continue;

        float depth = shadowMapTexture.SampleLevel(blockerSampler, float3(sampleUV, float(slice)), 0);

        if (depth < compareDepth)
        {
            blockerDepthSum += depth;
            blockerCount += 1.0;
        }
    }

    if (blockerCount == 0.0)
    {
        return 1.0;
    }

    float averageBlockerDepth = blockerDepthSum / blockerCount;
    float penumbra = max(0.0, (receiverDepth - averageBlockerDepth) / max(averageBlockerDepth, 0.001));
    float filterRadius = min(LIGHT_SIZE_UV * penumbra, MAX_FILTER_RADIUS_UV);
    filterRadius = max(filterRadius, 1.0 / shadowMapSize);

    float visibility = 0.0;

    [unroll]
    for (int j = 0; j < PCF_SAMPLES; ++j)
    {
        float2 sampleUV = uv + PoissonDisk[j] * filterRadius;

        visibility += shadowMapTexture.SampleCmpLevelZero(pcssSampler, float3(sampleUV, float(slice)), compareDepth);
    }

    return visibility / PCF_SAMPLES;
}

float CalculateCSMShadow(float3 worldPosition, float3 worldNormal, float3 lightDirection, float cameraDepth, float CascadeSplits[NUM_CASCADES], float4x4 LightViewProjections[NUM_CASCADES], Texture2DArray<float> shadowMaps, SamplerState blockerSampler, SamplerComparisonState pcssSmp, float shadowMapSize)
{
    uint cascadeIndex = 0;
    for (uint i = 0; i < NUM_CASCADES; ++i)
    {
        if (cameraDepth <= CascadeSplits[i])
        {
            cascadeIndex = i;
            break;
        }
        cascadeIndex = i;
    }

    float NoL = saturate(dot(normalize(worldNormal), normalize(lightDirection)));
    return CalculateShadow(worldPosition, LightViewProjections[cascadeIndex], shadowMaps, cascadeIndex, blockerSampler, pcssSmp, NoL, shadowMapSize);
}