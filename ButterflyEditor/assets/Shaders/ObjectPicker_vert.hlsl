struct CameraData
{
    float4x4 ViewProjection;
    float3 CameraPosition;
};

struct ModelMatrix
{
    float4x4 ModelMatrix;
    float3x3 NormalMatrix;
    float3 BoundsMin;
    float3 BoundsSize;
    float3 SDFResolution;
};

struct BufferIndices
{
    int positionBuffer;
    int uniformIndex;
    int modelIndex;
    int entityRenderIndex;
    int entitySceneIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    uint color : COLOR0;
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> position = ResourceDescriptorHeap[resources.positionBuffer];
    ConstantBuffer<CameraData> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    StructuredBuffer<ModelMatrix> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, modelMatrices[resources.entityRenderIndex].ModelMatrix);
    
    V2P output;
    output.color = resources.entitySceneIndex + 1;
    output.position = mul(MVP, float4(position[vertexID], 1.0f));
    return output;
}