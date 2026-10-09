#define PI 3.14159265359
#define EPSILON 0.0001
#define FLT_MAX 3.402823466e+38F

float3 F_Schlick(float3 F0, float VoH)
{
    return F0 + (1.0 - F0) * pow(1.0 - VoH, 5.0);
}

float D_GGX(float alphaRoughness, float NoH)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = NoH * NoH * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (PI * f * f);
}

float V_GGX(float alphaRoughness, float NoV, float NoL)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NoL * sqrt(NoV * NoV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NoV * sqrt(NoL * NoL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

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

bool IsInsideAABB(float3 position, float3 min, float3 max)
{
    return position.x >= min.x && position.x <= max.x &&
           position.y >= min.y && position.y <= max.y &&
           position.z >= min.z && position.z <= max.z;
}

float2 RayAABBIntersection(float3 rayOrigin, float3 rayDirection, float3 boundsMin, float3 boundsMax)
{
    float3 invDir = 1.0 / rayDirection;
    float3 tMin = (boundsMin - rayOrigin) * invDir;
    float3 tMax = (boundsMax - rayOrigin) * invDir;
    float tNear = max(max(min(tMin.x, tMax.x), min(tMin.y, tMax.y)), min(tMin.z, tMax.z));
    float tFar = min(min(max(tMin.x, tMax.x), max(tMin.y, tMax.y)), max(tMin.z, tMax.z));
    return float2(tNear, tFar);
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
};

ConstantBuffer<BufferIndices> resources : register(b0);

float SoftShadow(float3 origin, float3 dir, float3 boundsMin, float3 boundsSize, int sdf, float mint, float maxt, float w)
{
    float res = 1.0;
    float ph = 1e20;

    float t = mint;
                
    for (uint step = 0; step < 128 && t < maxt; step++)
    {
        float3 position = origin + dir * t;
        float3 uvw = (position - boundsMin) / boundsSize;

        Texture3D<float> sdfTexture = ResourceDescriptorHeap[sdf];
        SamplerState sdfSampler = SamplerDescriptorHeap[resources.sdfSamplerIndex];
        float h = sdfTexture.SampleLevel(sdfSampler, uvw, 0);
        if(h < 0.001)
            return 0.0;

        float y = h * h / (2.0 * ph);
        float d = sqrt(h * h - y * y);
        res = min(res, d / (w * max(0.0, t - y)));
        t += h;
        ph = h;
    }
    return res;
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
    
    if (resources.numLights > 0)
    {
        Light light = lights[0];
        if (light.Type == 0)
        {
            float3 rayDirection = -light.Direction;
            float3 rayOrigin = pixelInput.fragPos + rayDirection * EPSILON;
            
            float mint[8];
            float maxt[8];
            int intersectingModels[8];
            float3 rayOriginModel[8];
            float3 rayDirectionModel[8];
            
            int numIntersectingModels = 0;
            for (int i = 0; i < resources.numModels; i++)
            {
                intersectingModels[numIntersectingModels] = -1;
                
                ModelData model = modelData[i];
                    
                if (model.SDFTextureIndex < 0 || i == resources.entityIndex)
                {
                    continue;
                }
                
                rayOriginModel[numIntersectingModels] = mul(model.InverseModelMatrix, float4(rayOrigin, 1.0)).xyz;
                rayDirectionModel[numIntersectingModels] = normalize(mul((float3x3) model.InverseModelMatrix, rayDirection));
                float2 tNearFar = RayAABBIntersection(rayOriginModel[numIntersectingModels], rayDirectionModel[numIntersectingModels], model.BoundsMin.xyz, model.BoundsMin.xyz + model.BoundsSize.xyz);

                if (tNearFar.x <= tNearFar.y && tNearFar.y >= 0.0)
                {
                    float tNear = max(tNearFar.x, 0.0);
                        
                    if (tNear < tNearFar.y)
                    {
                        mint[numIntersectingModels] = tNear;
                        maxt[numIntersectingModels] = tNearFar.y;
                        intersectingModels[numIntersectingModels] = i;
                        numIntersectingModels++;
                    }
                }
            }
            
            float shadow = 1.0;
            float w = 0.1;
            for (int i = 0; i < numIntersectingModels; i++)
            {
                float lightAmount = SoftShadow(rayOriginModel[i], 
                rayDirectionModel[i], 
                modelData[intersectingModels[i]].BoundsMin.xyz, 
                modelData[intersectingModels[i]].BoundsSize.xyz, 
                modelData[intersectingModels[i]].SDFTextureIndex, 
                mint[i], maxt[i], w);
                
                shadow *= lightAmount;
                
                if (shadow <= EPSILON)
                {
                    return float4(fragShadowColor, 1.0);
                }
            }
            return float4(fragColor * shadow, 1.0);
        }
    }
    
    return float4(fragColor, 1.0);
}