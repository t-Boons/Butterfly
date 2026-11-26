#include "Common.hlsli"

struct Indices
{
    int atmosphere;
    int irradiance;
    int smp;
};

ConstantBuffer<Indices> resources : register(b0);

float3 GetDirection(uint face, float2 uv)
{
    uv = uv * 2.0f - 1.0f;
    switch (face)
    {
        case 0:  return normalize(float3( 1.0f, -uv.y, -uv.x)); // +X
        case 1:  return normalize(float3(-1.0f, -uv.y,  uv.x)); // -X
        case 2:  return normalize(float3( uv.x,  1.0f,  uv.y)); // +Y
        case 3:  return normalize(float3( uv.x, -1.0f, -uv.y)); // -Y
        case 4:  return normalize(float3( uv.x, -uv.y,  1.0f)); // +Z
        default: return normalize(float3(-uv.x, -uv.y, -1.0f)); // -Z
    }
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    TextureCube<float4> atmosphereTex = ResourceDescriptorHeap[resources.atmosphere];
    RWTexture2DArray<float4> irradianceTex = ResourceDescriptorHeap[resources.irradiance];
    SamplerState smp = SamplerDescriptorHeap[resources.smp];

    uint width, height, faces;
    irradianceTex.GetDimensions(width, height, faces);
    if (id.x >= width || id.y >= height)
        return;

    float2 uv = (id.xy + 0.5f) / float2(width, height);
    float3 normal  = GetDirection(id.z, uv);

    float3 up = abs(normal.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    const float sampleDelta = 0.025f;
    const float mip = 4.0f;
    
    float3 irradiance = 0;
    float count = 0;

    for (float phi = 0; phi < 2.0f * PI; phi += sampleDelta)
    {
        for (float theta = 0; theta < 0.5f * PI; theta += sampleDelta)
        {
            float3 t = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            float3 dir = t.x * right + t.y * up + t.z * normal;

            irradiance += atmosphereTex.SampleLevel(smp, dir, mip).rgb * cos(theta) * sin(theta);
            count++;
        }
    }

    irradiance = irradiance / count;
    irradianceTex[id] = float4(irradiance, 1.0f);
}