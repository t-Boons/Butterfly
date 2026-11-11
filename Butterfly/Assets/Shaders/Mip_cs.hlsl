struct ButterIndices
{
    int tex;
    int dst;
    int resX;
    int resY;
    int srcMip;
};

ConstantBuffer<ButterIndices> resources : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= resources.resX || id.y >= resources.resY)
    {
        return;
    }

    Texture2D<float4> src = ResourceDescriptorHeap[resources.tex];
    RWTexture2D<float4> dst = ResourceDescriptorHeap[resources.dst];

    uint2 srcCoord = id.xy * 2;

    float4 result = 0.0;

    [unroll]
    for (uint y = 0; y < 2; y++)
    {
        [unroll]
        for (uint x = 0; x < 2; x++)
        {
            result += src.Load(int3(srcCoord + uint2(x, y), resources.srcMip));
        }
    }

    dst[id.xy] = result * 0.25;
}