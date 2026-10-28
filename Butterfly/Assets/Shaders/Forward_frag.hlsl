#include "Brdf.hlsli"
#include "Sdf.hlsli"
#include "Shadow.hlsli"

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
    int blockerSamplerIndex;
    int pcssShadowSamplerIndex;
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
    float depth : DEPTH;
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

    float3 ambient = float3(0.05, 0.05, 0.07);
   
    float3 lighting = ambient * diffuseColor;
    
    SamplerComparisonState pcssShadowSmp = SamplerDescriptorHeap[resources.pcssShadowSamplerIndex];
    SamplerState blockerSmp = SamplerDescriptorHeap[resources.blockerSamplerIndex];
    
    for (int i = 0; i < resources.numLights; i++)
    {
        Light light = lights[i];
        
        if (light.Type == 0) // Directional Light
        {
//#if USE_PCSS_SHADOWS
            ShadowMap shadowMap = light.Shadow;
            
            float shadow = 1.0f;
            if (shadowMap.ShadowMapTextureIndex >= 0)
            {
                Texture2DArray<float> shadowMapTexture = ResourceDescriptorHeap[shadowMap.ShadowMapTextureIndex];
            
                float2 seed = pixelInput.position.xy;
                shadow = CalculateCSMShadow(pixelInput.fragPos, pixelInput.normal, -light.Direction, pixelInput.depth, shadowMap.CascadeSplits, shadowMap.CascadeTexelSize, shadowMap.LightViewProjections, shadowMapTexture, blockerSmp, pcssShadowSmp, shadowMap.Size, seed);
            }

            
            if (shadow <= 0.0)
            {
                continue;
            }
//#endif
            
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

#if 1
            lighting += (diffuseBRDF + specularBRDF) * NoL * light.Color * shadow;
#else
            lighting += (diffuseBRDF + specularBRDF) * NoL * light.Color;
#endif
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
    
#if USE_SDF_SHADOWS
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
    
                float3 boundMin = model.SDFBoundsMin.xyz;
                float3 boundSize = model.SDFBoundsSize.xyz;
            
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
        
        fragColor = lerp(ambient * diffuseColor, fragColor, shadow);
    }
#endif
    
    return float4(fragColor, 1.0);
}