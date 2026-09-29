

// Functions

float3 MapNormal(float3 sampledNormal, float3 vertexNormal, float3 vertexTangent, float tangentSign)
{
    float3 N = normalize(vertexNormal);

    float3 T = normalize(vertexTangent);
    T = normalize(T - N * dot(N, T));

    float3 B = cross(N, T) * tangentSign;

    float3 tangentNormal = sampledNormal * 2.0 - 1.0;

    return normalize(
        T * tangentNormal.x +
        B * tangentNormal.y +
        N * tangentNormal.z
    );
}


struct MaterialData
{
    float4 BaseColor;
    float4 EmissiveColor;
    float Metallic;
    float Roughness;
    float NormalScale;
    int ColorTexture;
    int NormalTexture;
    int MetallicRoughnessTexture;
    int EmissionTexture;
    int AmbientOcclusionTexture;
};

struct Light
{
    uint Type;
    float3 Color;
    float3 Position;
    float3 Direction;
    float Range;
    float InnerConeAngleCos;
    float OuterConeAngleCos;
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
};


float4 main(V2P pixelInput) : SV_TARGET0
{
    SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];
    StructuredBuffer<Light> lights = ResourceDescriptorHeap[resources.lightBuffer];
    StructuredBuffer<MaterialData> materials = ResourceDescriptorHeap[resources.materialBuffer];
    
    MaterialData material = materials[resources.materialIndex];
    
    float3 normal = normalize(pixelInput.normal);
    if (material.NormalTexture >= 0)
    {
        Texture2D<float4> normalTex = ResourceDescriptorHeap[material.NormalTexture];
        float3 sampledNormal = normalTex.Sample(smp, pixelInput.texCoord).xyz;
        normal = MapNormal(sampledNormal, normal, pixelInput.tangent, pixelInput.tangentW);
    }
    
    float3 albedo = material.BaseColor.xyz;
    if (material.ColorTexture >= 0)
    {
        Texture2D<float4> albedoTex = ResourceDescriptorHeap[material.ColorTexture];
        albedo = albedoTex.Sample(smp, pixelInput.texCoord).xyz;
    }
    
    float2 metallicRoughness = float2(0.0, 1.0);
    if (material.MetallicRoughnessTexture >= 0)
    {
        Texture2D<float4> metallicTex = ResourceDescriptorHeap[material.MetallicRoughnessTexture];
        metallicRoughness.xy = metallicTex.Sample(smp, pixelInput.texCoord).xy;
    }
    
    float3 emissive = material.EmissiveColor.xyz;
    if (material.EmissionTexture >= 0)
    {
        Texture2D<float4> emissiveTex = ResourceDescriptorHeap[material.EmissionTexture];
        emissive = emissiveTex.Sample(smp, pixelInput.texCoord).xyz;
    }

    float3 lightDir = normalize(float3(0.5, 1.0, 0.0f));
    float3 ambient = 0.1 * albedo;
    
    float3 lighting = ambient;
    
    for (int i = 0; i < resources.numLights; i++)
    {
        Light light = lights[i];
       
        if(light.Type == 0) // Directional Light
        {
            float3 lightDir = normalize(light.Direction);
            float3 diffuse = max(dot(normal, lightDir), 0.0) * light.Color * albedo;
            lighting += diffuse;
            continue;
        }
        
        float distance = length(light.Position - pixelInput.fragPos);
        if (distance >= light.Range)
        {
            continue;
        }
        
        float3 lightDir = normalize(light.Position - pixelInput.fragPos);
        float inverseSquare = 1.0 / max(distance * distance, 0.0001);
        float rangeFade = 1.0 - saturate(distance / light.Range);
        rangeFade *= rangeFade;
        float attenuation = inverseSquare * rangeFade;
        
        if (light.Type == 1) // Point Light
        {
            float3 diffuse = max(dot(normal, lightDir), 0.0) * light.Color * albedo * attenuation;
            lighting += diffuse;
            continue;
        }
        
        if(light.Type == 2) // Spot light
        {
            float coneAttenuation = smoothstep(light.OuterConeAngleCos, light.InnerConeAngleCos, dot(-lightDir, normalize(light.Direction)));
            float3 diffuse = max(dot(normal, lightDir), 0.0) * light.Color * albedo * attenuation * coneAttenuation;
            lighting += diffuse;   
            continue;
        }
    }
    
    return float4(lighting.xyz + emissive, 1.0f);
}