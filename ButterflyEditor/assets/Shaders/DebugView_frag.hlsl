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


float4 main(V2P pixelInput) : SV_TARGET0
{
    SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];
    StructuredBuffer<Light> lights = ResourceDescriptorHeap[resources.lightBuffer];
    StructuredBuffer<MaterialData> materials = ResourceDescriptorHeap[resources.materialBuffer];
    
    MaterialData material = materials[resources.materialIndex];
    
    float metallic = material.Metallic;
    float perceptualRoughness = material.Roughness;
    if (material.MetallicRoughnessTexture >= 0)
    {
        Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[material.MetallicRoughnessTexture];
        float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(smp, pixelInput.texCoord);
        metallic = metallicRoughnessSample.z;
        perceptualRoughness = metallicRoughnessSample.y;
    }
    metallic = saturate(metallic);
    perceptualRoughness = max(perceptualRoughness, 0.045);
    float alphaRoughness = perceptualRoughness * perceptualRoughness;
    


    if (resources.debugViewIndex == 1) // Normals
    {
        float3 normal = normalize(pixelInput.normal);
        if (material.NormalTexture >= 0)
        {
            Texture2D<float4> normalTex = ResourceDescriptorHeap[material.NormalTexture];
            float3 sampledNormal = normalTex.Sample(smp, pixelInput.texCoord).xyz;
            normal = MapNormal(sampledNormal, normal, pixelInput.tangent, pixelInput.tangentW);
        }
        
        return float4(normal * 0.5 + 0.5, 1.0f);
    }
    else if (resources.debugViewIndex == 2) // Albedo
    {
        float3 albedo = material.BaseColor.xyz;
        if (material.ColorTexture >= 0)
        {
            Texture2D<float4> albedoTex = ResourceDescriptorHeap[material.ColorTexture];
            albedo = albedoTex.Sample(smp, pixelInput.texCoord).xyz;
        }
        
        return float4(albedo, 1.0f);
    }
    else if (resources.debugViewIndex == 3) // Roughness
    {
        return float4(perceptualRoughness, perceptualRoughness, perceptualRoughness, 1.0f);
    }
    else if (resources.debugViewIndex == 4) // Metallic
    {
        
        return float4(metallic, metallic, metallic, 1.0f);
    }
    else if (resources.debugViewIndex == 5) // Emission
    {
        float3 emissive = material.EmissiveColor.xyz;
        if (material.EmissionTexture >= 0)
        {
            Texture2D<float4> emissiveTex = ResourceDescriptorHeap[material.EmissionTexture];
            emissive = emissiveTex.Sample(smp, pixelInput.texCoord).xyz;
        }
        
        return float4(emissive, 1.0f);
    }
    else if (resources.debugViewIndex == 6) // UVs
    {
        return float4(pixelInput.texCoord, 0.0f, 1.0f);
    }
    else if (resources.debugViewIndex == 7) // SDF Distance
    {
        float sdfDistance = 1.0f;
        if (resources.sdfTextureIndex >= 0)
        {
            Texture3D<float> sdfTexture = ResourceDescriptorHeap[resources.sdfTextureIndex];
            SamplerState smp = SamplerDescriptorHeap[resources.samplerIndex];
            sdfDistance = sdfTexture.Sample(smp, pixelInput.sdfUVW);
            
            float3 color = lerp(float3(0.0f, 1.0f, 0.0f), float3(1.0f, 0.0f, 0.0f), saturate(sdfDistance * 10.0f));
            return float4(color, 1.0f);

        }
        
        return float4(1.0, 0.0, 1.0, 1.0);
    }
    
    return float4(1.0f, 1.0f, 0.0f, 1.0f);
}