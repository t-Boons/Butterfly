#define PI 3.14159265359
#define EPSILON 1e-5

float3 F_Schlick(float3 F0, float VoH)
{
    float f = pow(1.0 - VoH, 5.0);
    return F0 + (1.0 - F0) * f;
}

float D_GGX(float roughness, float NoH)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = (NoH * a2 - NoH) * NoH + 1;
    return a2 / (PI * d * d + EPSILON);
}

float G1_SmithGGX(float roughness, float NoX)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NoX2 = NoX * NoX;

    return (2.0 * NoX) / (NoX + sqrt(NoX2 + a2 * (1.0 - NoX2)));
}

float G_SmithGGX(float roughness, float NoV, float NoL)
{
    float Gv = G1_SmithGGX(roughness, NoV);
    float Gl = G1_SmithGGX(roughness, NoL);

    return Gv * Gl;
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
    float roughness = material.Roughness;
    if (material.MetallicRoughnessTexture >= 0)
    {
        Texture2D<float4> metallicRoughnessTexture = ResourceDescriptorHeap[material.MetallicRoughnessTexture];
        float4 metallicRoughnessSample = metallicRoughnessTexture.Sample(smp, pixelInput.texCoord);
        metallic = metallicRoughnessSample.z;
        roughness = metallicRoughnessSample.y;
    }
    metallic = saturate(metallic);
    roughness = max(roughness, 0.045);
    
    float3 emissive = material.EmissiveColor.xyz;
    if (material.EmissionTexture >= 0)
    {
        Texture2D<float4> emissiveTex = ResourceDescriptorHeap[material.EmissionTexture];
        emissive = emissiveTex.Sample(smp, pixelInput.texCoord).xyz;
    }

        float3 lightDir = normalize(float3(0.5, 1.0, 0.0f));
    float3 ambient = 0.1 * albedo;
    
    float3 lighting = ambient;
    
    float3 N = normal;
    float3 V = normalize(pixelInput.eye - pixelInput.fragPos);
    float NoV = saturate(dot(N, V));
    
    for (int i = 0; i < resources.numLights; i++)
    {
        Light light = lights[i];
       
        if(light.Type == 0) // Directional Light
        {
            float3 L = normalize(-light.Direction);
            float3 H = normalize(L + V);
            float NoL = saturate(dot(N, L));
            float NoH = saturate(dot(N, H));
            float VoH = saturate(dot(V, H));
            
            float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
            float3 F = F_Schlick(F0, VoH);
            float3 D = D_GGX(roughness, NoH);
            float3 G = G_SmithGGX(roughness, NoV, NoL);
            float3 specular = D * F * G / (4.0 * NoV * NoL + EPSILON);
            float3 kD = (1.0 - F) * (1.0 - metallic);
            float3 diffuse = kD * albedo / PI;
            lighting += (diffuse + specular) * light.Color * NoL;
            
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
        float3 D = D_GGX(roughness, NoH);
        float3 G = G_SmithGGX(roughness, NoV, NoL);
        float3 specular = D * F * G / (4.0 * NoV * NoL + EPSILON);
        float3 kD = (1.0 - F) * (1.0 - metallic);
        float3 diffuse = kD * albedo / PI;
        
        if (light.Type == 1) // Point Light
        {
            float attenuation = inverseSquare * rangeFade;
            lighting += (diffuse + specular) * light.Color * NoL * attenuation;
            
            continue;
        }
        
        if(light.Type == 2) // Spot light
        {
            float cone = smoothstep(light.OuterConeAngleCos, light.InnerConeAngleCos, dot(-L, normalize(light.Direction)));
            float attenuation = inverseSquare * rangeFade * cone;
            lighting += (diffuse + specular) * light.Color * NoL * attenuation;
            continue;
        }
    }
    
    return float4(lighting.xyz + emissive, 1.0f);
}