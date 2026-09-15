struct BufferIndices
{
    int cameraUniforms;
    int skyboxSampler;
    int skyboxTexture;
};

struct SkyboxCameraData
{
    float4x4 invView;
    float4x4 invProjection;
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

    ConstantBuffer<SkyboxCameraData> uniforms = ResourceDescriptorHeap[resources.cameraUniforms];
    SamplerState skyboxSampler = ResourceDescriptorHeap[resources.skyboxSampler];
    TextureCube<float4> skybox = ResourceDescriptorHeap[resources.skyboxTexture];
    
    float4 viewPosition = mul(float4(ndc, 1.0, 1.0), uniforms.invProjection);
    float3 viewDirection = normalize(viewPosition.xyz / viewPosition.w);
    float3 worldDirection = normalize(mul(float4(viewDirection, 0.0), uniforms.invView).xyz);

    return skybox.Sample(skyboxSampler, worldDirection);
}