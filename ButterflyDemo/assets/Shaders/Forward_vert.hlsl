struct Uniforms
{
    float4x4 ViewProjection;
    float4x4 InvViewProjection;
    float4x4 Model;
};

struct BufferIndices
{
    int positionBuffer;
    int normalBuffer;
    int texcoordBuffer;
    int uniformIndex;
    int samplerIndex;
    int textureIndex;
    int lightsIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    nointerpolation int samplerIndex : SAMPLERINDEX;
    nointerpolation int textureIndex : TEXTUREINDEX;
    nointerpolation int lightIndex : LIGHTINDEX;
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> position = ResourceDescriptorHeap[resources.positionBuffer];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[resources.normalBuffer];
    StructuredBuffer<float2> texcoords = ResourceDescriptorHeap[resources.texcoordBuffer];
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, uniforms.Model);
    
    V2P output;
    output.position = mul(MVP, float4(position[vertexID], 1.0));
    output.normal = normalize(mul((float3x3) uniforms.Model, normals[vertexID]));
    output.texCoord = texcoords[vertexID];
    output.samplerIndex = resources.samplerIndex;
    output.textureIndex = resources.textureIndex;
    output.lightIndex = resources.lightsIndex;
    output.fragPos = mul(uniforms.Model, float4(position[vertexID], 1.0)).xyz;
    return output;
}