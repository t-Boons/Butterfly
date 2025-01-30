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
    nointerpolation int lightIndex : LIGHTINDEX;
};


float4 main(V2P pixelInput) : SV_TARGET0
{
    SamplerState smp = SamplerDescriptorHeap[pixelInput.samplerIndex];
    Texture2D<float4> tex = ResourceDescriptorHeap[pixelInput.textureIndex];
    StructuredBuffer<PointLight> lights = ResourceDescriptorHeap[pixelInput.lightIndex];
    
    float3 normal = pixelInput.normal;
    float3 albedo = tex.Sample(smp, pixelInput.texCoord).xyz;
    
    uint numLights = 0;
    uint stride = 0;
    lights.GetDimensions(numLights, stride);
    
    float3 lighting = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < numLights; ++i)
    {
        // diffuse
        float3 lightDir = normalize(lights[i].position - pixelInput.fragPos);
        float distance = length(lights[i].position - pixelInput.fragPos);
        float falloff = 1.0 / (distance * distance);
        if (falloff < 0.01) continue;
        float3 diffuse = max(dot(normal, lightDir), 0.0) * lights[i].color * lights[i].intensity * falloff;
        lighting += diffuse * albedo;
    }
    
    return float4(lighting, 1.0f);
}