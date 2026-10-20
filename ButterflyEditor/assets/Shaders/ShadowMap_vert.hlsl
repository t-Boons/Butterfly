#include "common.hlsli"
#define NUM_CASCADES 4

struct ShadowLightSource
{
    float4x4 ViewProj[NUM_CASCADES];
};

struct BufferIndices
{
    int positionBuffer;
    int cameraDataIndex;
    int shadowLightSourceIndex;
    int modelDataBufferIndex;
    int entityIndex;
    int cascadeIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
};

V2P main(uint vertexID : SV_VertexID)
{
    ConstantBuffer<CameraData> cameraData = ResourceDescriptorHeap[resources.cameraDataIndex];
    ConstantBuffer<ShadowLightSource> lightSource = ResourceDescriptorHeap[resources.shadowLightSourceIndex];
    StructuredBuffer<ModelData> modelMatrices = ResourceDescriptorHeap[resources.modelDataBufferIndex];
    StructuredBuffer<float3> position = ResourceDescriptorHeap[resources.positionBuffer];
    float4x4 MVP = mul(lightSource.ViewProj[resources.cascadeIndex], modelMatrices[resources.entityIndex].ModelMatrix);
    
    V2P output;
    output.position = mul(MVP, float4(position[vertexID], 1.0f));
    return output;
}