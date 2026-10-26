
struct ButterIndices
{
    int sdfOutIndex;
    int triangleBufferIndex;
};

ConstantBuffer<ButterIndices> resources : register(b0);

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWTexture3D<float4> outTexture = ResourceDescriptorHeap[resources.sdfOutIndex];
    Texture2D<float4> dst = ResourceDescriptorHeap[resources.dst];

}