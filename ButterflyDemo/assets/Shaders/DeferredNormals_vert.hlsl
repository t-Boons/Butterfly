struct Uniforms
{
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4x4 Model;
};

struct BufferIndices
{
    int positionIndex;
    int normalIndex;
    int uniformIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float3 normal : NORMAL0;
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[resources.positionIndex];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[resources.normalIndex];
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, uniforms.Model);
    
    V2P output;
    float4 pos = mul(MVP, float4(positions[vertexID], 1.0));
    output.position = pos;
    output.normal = normalize(mul((float3x3) uniforms.Model, normals[vertexID]));
    return output;
}