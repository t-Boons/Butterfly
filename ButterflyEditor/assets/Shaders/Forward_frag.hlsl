struct Light
{
    uint Type;
    float3 Color;
    float Range;
    float ConeAngle;
    float3 Direction;
    float3 Position;
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
    int lightBuffer;
    int numLights;
};

ConstantBuffer<BufferIndices> resources : register(b0);


struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};


float4 main(V2P pixelInput) : SV_TARGET0
{
    SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];
    Texture2D<float4> tex = ResourceDescriptorHeap[resources.textureIndex];
    StructuredBuffer<Light> lights = ResourceDescriptorHeap[resources.lightBuffer];
    
    float3 normal = pixelInput.normal;
    float3 albedo = tex.Sample(smp, pixelInput.texCoord).xyz;
    

    float3 lightDir = normalize(float3(0.5, 1.0, 0.0f));
    float3 ambient = 0.1 * albedo;
    
    float3 lighting = ambient;
    
    for (int i = 0; i < resources.numLights; i++)
    {
        Light light = lights[i];
        float3 lightDir = normalize(light.Position - pixelInput.fragPos);
        float3 diffuse = max(dot(normal, lightDir), 0.0) * light.Color * albedo;
        lighting += diffuse;
    }
    
    return float4(lighting, 1.0f);
}