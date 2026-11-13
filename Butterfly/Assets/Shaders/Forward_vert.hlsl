#include "Common.hlsli"
#include "Mesh.hlsli"
#include "FrameData.hlsli"
#include "Model.hlsli"
#include "Camera.hlsli"

struct BufferIndices
{
    int frameDataIndex;

    int meshIndex;
    int modelIndex;
    int materialIndex;

    int skyboxTextureIndex;

    int sdfSamplerIndex;
    int blockerSamplerIndex;
    int pcssShadowSamplerIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texCoord : TEXCOORD0;
};

V2P main(uint vertexID : SV_VertexID)
{
    FrameData frameData = GetFrameData(resources.frameDataIndex);
    CameraData camera = frameData.GetCameraData();
    ModelData model = frameData.GetModelData(resources.modelIndex);

    Mesh mesh = frameData.GetMesh(resources.meshIndex);
    float3 position = mesh.GetPosition(vertexID);
    float3 normal = mesh.GetNormal(vertexID);
    float4 tangent = mesh.GetTangent(vertexID);
    float2 texCoord = mesh.GetUV(vertexID);
    


    float4 worldPos = mul(model.ModelMatrix, float4(position, 1.0));

    V2P output;
    output.position =  mul(camera.ViewProj, worldPos);
    output.normal = mul((float3x3) model.NormalMatrix, normal);
    output.tangent = float4(mul((float3x3)model.ModelMatrix, tangent.xyz), tangent.w);
    output.texCoord = texCoord;
    output.fragPos = worldPos.xyz;
    return output;
}