#include "common.hlsli"

struct BufferIndices
{
    int cameraUniforms;
    int skyboxSampler;
    int skyboxTexture;
};

ConstantBuffer<BufferIndices> resources : register(b0);

struct V2P
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

float4 main(V2P input) : SV_Target
{
    float2 ndc = float2(input.UV.x * 2.0 - 1.0, 1.0 - input.UV.y * 2.0);
    
    ConstantBuffer<CameraData> uniforms = ResourceDescriptorHeap[resources.cameraUniforms];
    SamplerState skyboxSampler = ResourceDescriptorHeap[resources.skyboxSampler];
    TextureCube<float4> skybox = ResourceDescriptorHeap[resources.skyboxTexture];
    
    float4 viewPosition = mul(float4(ndc, 1.0, 1.0), uniforms.InvProj);
    float3 viewDirection = normalize(viewPosition.xyz / viewPosition.w);
    float3 worldDirection = normalize(mul(float4(viewDirection, 0.0), uniforms.InvView).xyz);

    return skybox.Sample(skyboxSampler, worldDirection);
}