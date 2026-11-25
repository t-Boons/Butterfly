struct ButterIndices
{
    int tex;
    int dst;
    int resX;
    int resY;
};

ConstantBuffer<ButterIndices> resources : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= resources.resX || id.y >= resources.resY)
    {
        return;
    }

    Texture2DArray<float4>   src = ResourceDescriptorHeap[resources.tex];
    RWTexture2DArray<float4> dst = ResourceDescriptorHeap[resources.dst];

    uint srcW, srcH, slices, levels;
    src.GetDimensions(0, srcW, srcH, slices, levels);

    uint2 srcCoord = id.xy * 2;
    float4 result = 0.0;

    [unroll]
    for (uint y = 0; y < 2; y++)
    {
        [unroll]
        for (uint x = 0; x < 2; x++)
        {
            uint2 c = min(srcCoord + uint2(x, y), uint2(srcW - 1, srcH - 1));
            result += src.Load(int4(c, 0, 0));
        }
    }

    dst[uint3(id.xy, 0)] = result * 0.25;
}