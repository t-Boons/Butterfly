#include "common.hlsli"

struct BufferIndices
{
    int positionBuffer;
    int normalBuffer;
    int tangentBuffer;
    int texcoordBuffer;
    int uniformIndex;
    int samplerIndex;
    int modelIndex;
    int entityIndex;
    int lightBuffer;
    int numLights;
    int materialBuffer;
    int materialIndex;
    int debugViewIndex;
    int sdfTextureIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    nointerpolation float tangentW : TANGENTW;
    float2 texCoord : TEXCOORD0;
    float3 eye : VIEWDIR;
    float3 sdfUVW : UVW;
};

V2P main(uint vertexID : SV_VertexID)
{
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[resources.positionBuffer];
    float3 position = positions[vertexID];
    
    float3 normal = float3(0.0, 0.0, 1.0);
    if (resources.normalBuffer >= 0)
    {
        StructuredBuffer<float3> normals = ResourceDescriptorHeap[resources.normalBuffer];
        normal = normals[vertexID];
    }
    
    float4 tangent = float4(0.0, 0.0, 0.0, 1.0);
    if (resources.tangentBuffer >= 0)
    {
        StructuredBuffer<float4> tangents = ResourceDescriptorHeap[resources.tangentBuffer];
        tangent = tangents[vertexID];
    }
    
    float2 texCoord = float2(0.0, 0.0);
    if (resources.texcoordBuffer >= 0)
    {
        StructuredBuffer<float2> texcoords = ResourceDescriptorHeap[resources.texcoordBuffer];
        texCoord = texcoords[vertexID];
    }
    
    ConstantBuffer<CameraData> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    StructuredBuffer<ModelData> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProj, modelMatrices[resources.entityIndex].ModelMatrix);
    
    V2P output;
    output.position = mul(MVP, float4(position, 1.0));
    output.normal = mul((float3x3) modelMatrices[resources.entityIndex].NormalMatrix, normal);
    output.tangent.xyz = mul((float3x3) modelMatrices[resources.entityIndex].NormalMatrix, tangent.xyz);
    output.tangentW = tangent.w;
    output.texCoord = texCoord;
    output.fragPos = mul(modelMatrices[resources.entityIndex].ModelMatrix, float4(position, 1.0)).xyz;
    output.eye = uniforms.Position;
    return output;
}