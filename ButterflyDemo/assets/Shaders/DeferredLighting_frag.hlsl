struct PointLight
{
    float3 position;
    float3 color;
    float radius;
    float intensity;
};

struct Uniforms
{
    float4x4 ViewProjection;
    float4x4 InverseView;
    float4x4 Model;
    float4x4 InverseProjection;
    int2 Resolution;
};

struct BufferIndices
{
    int spherePositionIndex;
    int normalTextureIndex;
    int uniformsIndex;
    int samplerIndex;
    int lightsIndex;
    int albedoTexture;
};

ConstantBuffer<BufferIndices> resources : register(b0);


struct V2P
{
    float4 position : SV_Position;
    PointLight light : LIGHT;
};

float4 main(V2P pixelInput) : SV_TARGET
{
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformsIndex];
    
    float2 texCoord = pixelInput.position.xy / float2(uniforms.Resolution.x, uniforms.Resolution.y);

    SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];

    Texture2D<float4> normals = ResourceDescriptorHeap[resources.normalTextureIndex];
    float3 normal = normals.SampleLevel(smp, texCoord, 0).xyz;

    Texture2D<float4> albedoTex = ResourceDescriptorHeap[resources.albedoTexture];
    float3 albedo = albedoTex.SampleLevel(smp, texCoord, 0).xyz;
    
    float depth = 1.0f - normals.SampleLevel(smp, texCoord, 0).a;

    float4 ndc;
    ndc.x = texCoord.x * 2 - 1;
    ndc.y = (1 - texCoord.y) * 2 - 1;
    ndc.z = depth;
    ndc.w = 1.0f;
        
    float4 viewSpace = mul(uniforms.InverseProjection, ndc);
    viewSpace /= viewSpace.w;
    float4 world = mul(uniforms.InverseView, viewSpace);
    float3 position = world.xyz;
    
    
    PointLight light = pixelInput.light;
    float3 lightDir = normalize(light.position - position);
    float distance = length(light.position - position);
    float falloff = 1.0 / (distance * distance);
    float3 diffuse = max(dot(normal, lightDir), 0.0) * light.color * light.intensity * falloff;

    return float4(diffuse * albedo, 1.0f);
}