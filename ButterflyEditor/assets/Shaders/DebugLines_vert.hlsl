struct CameraData
{
    float4x4 ViewProjection;
    float3x3 NormalMatrix;
    float3 CameraPosition;
};

struct Vertex
{
    float3 Position;
    float4 Color;
};

struct BufferIndices
{
    int cameraDataIndex;
    int vertexIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

V2P main(uint vertexID : SV_VertexID)
{
    ConstantBuffer<CameraData> uniforms = ResourceDescriptorHeap[resources.cameraDataIndex];
    StructuredBuffer<Vertex> vertices = ResourceDescriptorHeap[resources.vertexIndex];

    float4x4 MVP = uniforms.ViewProjection;
    
    V2P output;
    output.color = vertices[vertexID].Color;
    output.position = mul(MVP, float4(vertices[vertexID].Position, 1.0f));
    return output;
}