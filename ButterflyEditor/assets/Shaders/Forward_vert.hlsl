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
    int normalBuffer;
    int texcoordBuffer;
    int uniformIndex;
    int samplerIndex;
    int textureIndex;
    int modelIndex;
    int entityIndex;
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
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> position = ResourceDescriptorHeap[resources.positionBuffer];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[resources.normalBuffer];
    StructuredBuffer<float2> texcoords = ResourceDescriptorHeap[resources.texcoordBuffer];
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    StructuredBuffer<ModelMatrix> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, modelMatrices[resources.entityIndex].modelMatrix);
    
    V2P output;
    output.position = mul(MVP, float4(position[vertexID], 1.0));
    output.normal = normalize(mul((float3x3) modelMatrices[resources.entityIndex].modelMatrix, normals[vertexID]));
    output.texCoord = texcoords[vertexID];
    output.samplerIndex = resources.samplerIndex;
    output.textureIndex = resources.textureIndex;
    output.fragPos = mul(modelMatrices[resources.entityIndex].modelMatrix, float4(position[vertexID], 1.0)).xyz;
    return output;
}