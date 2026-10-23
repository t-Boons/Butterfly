#define NUM_CASCADES 4

#define POISSON_SAMPLE_COUNT 16 
#define PCSS_BLOCKER_SEARCH 0.075
#define PCSS_LIGHT_SIZE 0.06


#define PCSS_MIN_PENUMBRA_SIZE 0.00075


static const float2 PCSS_Poisson_Samples[POISSON_SAMPLE_COUNT] =
{
    float2(-0.352, -0.698), float2(0.602, 0.798),
    float2(-0.794, 0.599), float2(0.887, -0.448),
    float2(-0.032, 0.143), float2(-0.982, -0.171),
    float2(-0.134, 0.970), float2(0.340, -0.907),
    float2(0.948, 0.228), float2(0.328, -0.302),
    float2(-0.466, -0.177), float2(0.444, 0.264),
    float2(-0.321, 0.523), float2(0.165, 0.630),
    float2(-0.777, -0.596), float2(-0.690, 0.182),
};

struct GPUShadowMap
{
    float4x4 LightViewProjections[NUM_CASCADES];
    float CascadeSplits[NUM_CASCADES];
    float CascadeTexelSize[NUM_CASCADES];
    int ShadowMapTextureIndex;
    int NumCascades;
    float Size;
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

inline float CalculateCSMShadow(float3 worldPosition, float3 worldNormal, float3 lightDirection, float cameraDepth, float cascadeSplits[NUM_CASCADES], float cascadeTexelSizes[NUM_CASCADES], float4x4 LightViewProjections[NUM_CASCADES], Texture2DArray<float> shadowMaps, SamplerState blockerSampler, SamplerComparisonState pcssSmp, float resolution, float2 rotationSeed)
{
    uint cascadeIndex = 0;
    for (uint i = 0; i < NUM_CASCADES; ++i)
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
    float cascadeRadius = 1.0 / length(lightVP[0].xyz);
    
    float texelWorld = cascadeTexelSizes[cascadeIndex];
    
    float sinTheta = sqrt(saturate(1.0 - NoL * NoL));
    
    float3 offsetPos = worldPosition + normalize(worldNormal) * texelWorld * 3.0 * sinTheta;
    float4 lightClip = mul(lightVP, float4(offsetPos, 1.0));
    float3 lightNDC = lightClip.xyz / lightClip.w;
    float2 uv = float2(lightNDC.x * 0.5 + 0.5, -lightNDC.y * 0.5 + 0.5);
    
    // Early out if the fragment is outside the shadow map bounds
    if (any(uv < 0.0) || any(uv > 1.0) || lightNDC.z < 0.0 || lightNDC.z > 1.0)
    {
        return 1.0;
    }
    
    float3 projectedPos = float3(uv, lightNDC.z - 0.005);

    return PCSS_Shadow(shadowMaps, cascadeIndex, blockerSampler, pcssSmp, projectedPos, PCSS_LIGHT_SIZE, rotationSeed);
}