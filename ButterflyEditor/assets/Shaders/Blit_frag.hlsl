struct BufferIndices
{
    int textureIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

float4 main(V2P input) : SV_Target
{
    Texture2D<float4> source = ResourceDescriptorHeap[resources.textureIndex];
    uint2 size;
    source.GetDimensions(size.x, size.y);
    int2 pixelCoord = int2(input.UV * size);
    return source.Load(int3(pixelCoord, 0), 0.0f);

}