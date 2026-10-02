static const float PI = 3.14159265359;

struct BufferIndices
{
    int hdriTexureIndex;
    int rwTexureIndex;
    int samplerIndex;
    int cubemapSize;
};

ConstantBuffer<BufferIndices> resources : register(b0);


float3 GetCubemapDirection(float2 uv, uint face)
{
    // Convert [0, 1] to [-1, 1]
    float2 p = uv * 2.0 - 1.0;

    switch (face)
    {
        case 0: return normalize(float3( 1.0, -p.y, -p.x)); // +X
        case 1: return normalize(float3(-1.0, -p.y,  p.x)); // -X
        case 2: return normalize(float3( p.x,  1.0,  p.y)); // +Y
        case 3: return normalize(float3( p.x, -1.0, -p.y)); // -Y
        case 4: return normalize(float3( p.x, -p.y,  1.0)); // +Z
        case 5: return normalize(float3(-p.x, -p.y, -1.0)); // -Z
    }

    return 0.0;
}

float2 DirectionToEquirectangularUV(float3 direction)
{
    float phi = atan2(direction.z, direction.x);
    float theta = asin(direction.y);

    float2 uv;

    uv.x = phi / (2.0 * PI) + 0.5;
    uv.y = theta / PI + 0.5;

    return uv;
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    uint face = id.z;

    if (pixel.x >= resources.cubemapSize ||
        pixel.y >= resources.cubemapSize ||
        face >= 6)
    {
        return;
    }
    
	SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];
	Texture2D<float4> hdriTex = ResourceDescriptorHeap[resources.hdriTexureIndex];
	RWTexture2DArray<float4> cubemap = ResourceDescriptorHeap[resources.rwTexureIndex];
    
    float2 uv = (float2(pixel) + 0.5) / float(resources.cubemapSize);
    float3 direction = GetCubemapDirection(uv, face);
    float2 hdriUV = DirectionToEquirectangularUV(direction);
    float3 color = hdriTex.Sample(smp, hdriUV).rgb;
	cubemap[uint3(pixel, face)] = float4(color, 1.0);
	}