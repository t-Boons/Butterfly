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
    
    float3 color = source.Load(int3(pixelCoord, 0)).xyz;
    
    // Convert linear color to sRGB
    for (int i = 0; i < 3; i++)
    {
        if (color[i] <= 0.0031308)
        {
            color[i] *= 12.92;
        }
        else
        {
            color[i] = 1.055 * pow(color[i], 1.0 / 2.4) - 0.055;
        }
    }
    
    return float4(color, 1.0f);
}