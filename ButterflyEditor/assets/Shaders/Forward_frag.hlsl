#include "brdf.hlsli"
#include "sdf.hlsli"

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
    int skyboxTextureIndex;
    int numModels;
    int sdfSamplerIndex;
};

ConstantBuffer<BufferIndices> resources : register(b0);

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

struct V2P
{
    float4 position : SV_Position;
    float3 fragPos : WORLDPOS;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    nointerpolation float tangentW : TANGENTW;
    float2 texCoord : TEXCOORD0;
    float3 eye : VIEWDIR;
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
    
    float3 emissive = material.EmissiveColor.xyz;
    if (material.EmissionTexture >= 0)
    {
        Texture2D<float4> emissiveTex = ResourceDescriptorHeap[material.EmissionTexture];
        emissive = emissiveTex.Sample(smp, pixelInput.texCoord).xyz;
    }

    float3 diffuseColor = albedo * (1.0 - metallic);
    float3 lightDir = normalize(float3(0.5, 1.0, 0.0f));
    
    float3 N = normal;
    float3 V = normalize(pixelInput.eye - pixelInput.fragPos);
    float NoV = saturate(dot(N, V));

    float3 ambient = float3(0.0, 0.0, 0.0);
    float3 reflection = float3(0.0, 0.0, 0.0);

    if (resources.skyboxTextureIndex >= 0)
    {
        TextureCube<float4> skyboxTex = ResourceDescriptorHeap[resources.skyboxTextureIndex];
        float3 skyColor = skyboxTex.Sample(smp, N).rgb;
        ambient = albedo * skyColor * 0.05f * (1.0 - metallic);

        float3 R = reflect(-V, N);
        reflection = skyboxTex.Sample(smp, R).rgb;
    }
    
    float3 skyLight = ambient + reflection * metallic * 0.01f;
    float3 lighting = skyLight;
    
    for (int i = 0; i < resources.numLights; i++)
    {
        Light light = lights[i];
        
        if (light.Type == 0) // Directional Light
        {
            float3 L = normalize(-light.Direction);
            float3 H = normalize(L + V);
            float NoL = saturate(dot(N, L));
            float NoH = saturate(dot(N, H));
            float VoH = saturate(dot(V, H));
            
            float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
            float3 F = F_Schlick(F0, VoH);
            float D = D_GGX(alphaRoughness, NoH);
            float Vis = V_GGX(alphaRoughness, NoV, NoL);

            float3 specularBRDF = F * Vis * D;
            float3 diffuseBRDF = diffuseColor / PI;

            lighting += (diffuseBRDF + specularBRDF) * NoL * light.Color;
            
            continue;
        }
        
        float distance = length(light.Position - pixelInput.fragPos);
        if (distance >= light.Range)
        {
            continue;
        }
        
        float inverseSquare = 1.0 / max(distance * distance, EPSILON);
        float rangeFade = 1.0 - saturate(distance / light.Range);
        rangeFade *= rangeFade;
        
        
        float3 L = normalize(light.Position - pixelInput.fragPos);
        float3 H = normalize(L + V);
        float NoL = saturate(dot(N, L));
        float NoH = saturate(dot(N, H));
        float VoH = saturate(dot(V, H));

        float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
        float3 F = F_Schlick(F0, VoH);
        float D = D_GGX(alphaRoughness, NoH);
        float Vis = V_GGX(alphaRoughness, NoV, NoL);

        float3 specularBRDF = F * Vis * D;
        float3 diffuseBRDF = diffuseColor / PI;


        
        if (light.Type == 1) // Point Light
        {
            float attenuation = inverseSquare * rangeFade;
            lighting += (diffuseBRDF + specularBRDF) * NoL * attenuation * light.Color;
            
            continue;
        }
        
        if (light.Type == 2) // Spot light
        {
            float cone = smoothstep(light.OuterConeAngleCos, light.InnerConeAngleCos, dot(-L, normalize(light.Direction)));
            float attenuation = inverseSquare * rangeFade * cone;
            lighting += (diffuseBRDF + specularBRDF) * NoL * attenuation * light.Color;
            continue;
        }
    }
    
    float3 fragColor = lighting.xyz + emissive * material.EmissiveColor.rgb;
    float3 fragShadowColor = skyLight;

    SamplerState sdfSampler = SamplerDescriptorHeap[resources.sdfSamplerIndex];
    StructuredBuffer<ModelData> modelData = ResourceDescriptorHeap[resources.modelIndex];
    
    float shadow = 1.0;
    float k = 32.0;
    for (int i = 0; i < resources.numLights; i++)
    {
        if (lights[i].Type == 0)
        {
            Light light = lights[i];

            float NoL = saturate(dot(normalize(pixelInput.normal), -light.Direction));
        
            for (int m = 0; m < resources.numModels; m++)
            {
                ModelData model = modelData[m];
                if (model.SDFTextureIndex < 0)
                    continue;

                float3 boundMin = model.BoundsMin.xyz;
                float3 boundSize = model.BoundsSize.xyz;
            
                float3 originW = pixelInput.fragPos + pixelInput.normal * EPSILON;
                float3 originM = mul(model.InverseModelMatrix, float4(originW, 1.0)).xyz;
                float3 dirM = normalize(mul(model.InverseModelMatrix, float4(-light.Direction, 0.0)).xyz);

                float margin = ShadowPenumbraMarginDirectional(originM, boundMin, boundSize, k);
                float2 tNF = RayAABBIntersection(originM, dirM, boundMin - margin, boundMin + boundSize + margin);

                bool isSelf = (m == resources.entityIndex);
            
                float fragmentDistanceToModel = SampleSDF(model.SDFTextureIndex, resources.sdfSamplerIndex, originM, boundMin, boundSize);

                bool startsInContact = isSelf || fragmentDistanceToModel < 0.05;
                float contactSkip = startsInContact ? 0.05 / max(NoL, 0.1) : 0.0;

                float minT = max(tNF.x, contactSkip);
                float maxT = tNF.y;
                
                if (minT >= maxT)
                    continue;

                float s = SoftShadowDirectional(originM, dirM, boundMin, boundSize, model.SDFTextureIndex, resources.sdfSamplerIndex, minT, maxT, k);

                if (isSelf)
                    s = lerp(1.0, s, smoothstep(0.0, 0.25, NoL));
            
                shadow *= s;
                if (shadow <= EPSILON)
                    break;
            }
        }
        
        return float4(lerp(fragShadowColor, fragColor, shadow), 1.0);
    }
    
    return float4(fragColor, 1.0);
}