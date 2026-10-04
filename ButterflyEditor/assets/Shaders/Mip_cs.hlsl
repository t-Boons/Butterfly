
struct ButterIndices
{
    uint2 src;
    uint2 dst;
};

ConstantBuffer<ButterIndices> resources : register(b0);

[numthreads(8, 8, 1)]
void CSMain(uint3 id : SV_DispatchThreadID)
{
    Texture2D<float4> src = ResourceDescriptorHeap[resources.src];
    Texture2D<float4> dst = ResourceDescriptorHeap[resources.dst];

    uint width;
    uint height;
    dst.GetDimensions(width, height);

    if (id.x >= width || id.y >= height)
        return;

    uint2 srcCoord = id.xy * 2;

    float4 a = src.Load(int3(srcCoord + uint2(0, 0), 0));
    float4 b = src.Load(int3(srcCoord + uint2(1, 0), 0));
    float4 c = src.Load(int3(srcCoord + uint2(0, 1), 0));
    float4 d = src.Load(int3(srcCoord + uint2(1, 1), 0));

    dst[id.xy] = (a + b + c + d) * 0.25;
}