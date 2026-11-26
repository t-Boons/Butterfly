#include "Common.hlsli"
#include "Brdf.hlsli"

#define  SAMPLE_COUNT 1024

struct Indices
{
    int   sky;
    int   prefilter;
    int   smp;
    float roughness;
    float size;
};

ConstantBuffer<Indices> resources : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    TextureCube<float4> sky = ResourceDescriptorHeap[resources.sky];
    RWTexture2DArray<float4> prefilter = ResourceDescriptorHeap[resources.prefilter];
    SamplerState smp = ResourceDescriptorHeap[resources.smp];

    uint w, h, faces;
    prefilter.GetDimensions(w, h, faces);
    if (id.x >= w || id.y >= h || id.z >= faces) return;

    float3 normal = GetDirection(id.z, (id.xy + 0.5f) / float2(w, h));
    float3 view = normal;

    float3 color  = 0;
    float  weight = 0;

    float saTexel = 4.0f * PI / (6.0f * resources.size * resources.size);

    float alphaRoughness = resources.roughness * resources.roughness;
    float roughness = resources.roughness;

    for (uint i = 0; i < SAMPLE_COUNT; i++)
    {
        float3 half = ImportanceSampleGGX(Hammersley(i, SAMPLE_COUNT), normal, alphaRoughness);
        float3 light = normalize(2.0f * dot(view, half) * half - view);

        float NoL = saturate(dot(normal, light));
        if (NoL > 0.0f)
        {
            float NoH = saturate(dot(normal, half));
            float pdf = D_GGX(alphaRoughness, NoH) * 0.25f + EPSILON;
            float saSample = 1.0f / (SAMPLE_COUNT * pdf);
            float mip = roughness == 0.0f ? 0.0f : 0.5f * log2(saSample / saTexel);
    
            color  += sky.SampleLevel(smp, light, mip).rgb * NoL;
            weight += NoL;
        }
    }

    prefilter[id] = float4(color / weight, 1.0f);
}