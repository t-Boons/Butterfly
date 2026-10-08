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
    StructuredBuffer<float3> position = ResourceDescriptorHeap[resources.positionBuffer];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[resources.normalBuffer];
    StructuredBuffer<float4> tangents = ResourceDescriptorHeap[resources.tangentBuffer];
    StructuredBuffer<float2> texcoords = ResourceDescriptorHeap[resources.texcoordBuffer];
    ConstantBuffer<CameraData> uniforms = ResourceDescriptorHeap[resources.uniformIndex];
    StructuredBuffer<ModelData> modelMatrices = ResourceDescriptorHeap[resources.modelIndex];
    
    float4x4 MVP = mul(uniforms.ViewProjection, modelMatrices[resources.entityIndex].ModelMatrix);
    
    V2P output;
    output.position = mul(MVP, float4(position[vertexID], 1.0));
    output.normal = mul((float3x3) modelMatrices[resources.entityIndex].NormalMatrix, normals[vertexID]);
    output.tangent.xyz = mul((float3x3) modelMatrices[resources.entityIndex].NormalMatrix, tangents[vertexID].xyz);
    output.tangentW = tangents[vertexID].w;
    output.texCoord = texcoords[vertexID];
    output.fragPos = mul(modelMatrices[resources.entityIndex].ModelMatrix, float4(position[vertexID], 1.0)).xyz;
    output.eye = uniforms.CameraPosition;
    output.sdfUVW = (position[vertexID] - modelMatrices[resources.entityIndex].BoundsMin.xyz) / modelMatrices[resources.entityIndex].BoundsSize.xyz;

    return output;
}