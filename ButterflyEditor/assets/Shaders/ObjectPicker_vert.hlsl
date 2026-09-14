struct Uniforms
{
    float4x4 ViewProjection;
};

struct ModelMatrix
{
    float4x4 modelMatrix;
};

struct BufferIndices
{
    int positionBuffer;
    int uniformIndex;
    int modelIndex;
    int entityIndex;
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
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    StructuredBuffer<ModelMatrix> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, modelMatrices[resources.entityIndex].modelMatrix);
    
    V2P output;
    output.color = resources.entityIndex + 1;
    output.position = mul(MVP, float4(position[vertexID], 1.0f));
    return output;
}