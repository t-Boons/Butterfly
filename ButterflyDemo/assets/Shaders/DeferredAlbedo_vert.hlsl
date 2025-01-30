struct Uniforms
{
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4x4 Model;
};

struct BufferIndices
{
    int positionIndex;
    int texCoordsIndex;
    int uniformIndex;
    int samplerIndex;
    int textureIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    nointerpolation int samplerIndex : SAMPLERINDEX;
    nointerpolation int textureIndex : TEXTUREINDEX;
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[resources.positionIndex];
    StructuredBuffer<float2> texCoords = ResourceDescriptorHeap[resources.texCoordsIndex];
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, uniforms.Model);
    
    V2P output;
    output.Position = mul(MVP, float4(positions[vertexID], 1));
    output.TexCoord = texCoords[vertexID];
    output.samplerIndex = resources.samplerIndex;
    output.textureIndex = resources.textureIndex;
    return output;
}