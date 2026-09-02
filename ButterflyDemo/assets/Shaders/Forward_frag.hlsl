struct PointLight
{
    float3 position;
    float3 color;
    float radius;
    float intensity;
};


struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    nointerpolation int samplerIndex : SAMPLERINDEX;
    nointerpolation int textureIndex : TEXTUREINDEX;
};


float4 main(V2P pixelInput) : SV_TARGET0
{
    SamplerState smp = SamplerDescriptorHeap[pixelInput.samplerIndex];
    Texture2D<float4> tex = ResourceDescriptorHeap[pixelInput.textureIndex];

    float3 normal = pixelInput.normal;
    float3 albedo = tex.Sample(smp, pixelInput.texCoord).xyz;
    

    float3 lightDir = normalize(float3(0.5, 1.0, 0.0f));
    float3 ambient = 0.1 * albedo;
    
    float3 diffuse = max(dot(normal, lightDir), 0.0);
    float3 lighting = diffuse * albedo + ambient;
    
    return float4(lighting, 1.0f);
}