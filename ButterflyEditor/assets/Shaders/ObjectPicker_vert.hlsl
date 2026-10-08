struct CameraData
{
    float4x4 ViewProjection;
    float3 CameraPosition;
};

struct ModelData
{
    float4x4 ModelMatrix;
    float4x4 InverseModelMatrix;
    float4x4 NormalMatrix;
    float4 BoundsMin;
    float4 BoundsSize;
    float4 SDFResolution;
    int SDFTextureIndex;
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
    StructuredBuffer<ModelData> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, modelMatrices[resources.entityRenderIndex].ModelMatrix);
    
    V2P output;
    output.color = resources.entitySceneIndex + 1;
    output.position = mul(MVP, float4(position[vertexID], 1.0f));
    return output;
}