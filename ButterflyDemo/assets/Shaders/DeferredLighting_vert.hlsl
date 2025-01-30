struct PointLight
{
    float3 position;
    float3 color;
    float radius;
    float intensity;
};

struct Vert
{
    float3 position;
    float2 texCoord;
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
    float4 Position : SV_Position;
    PointLight light : LIGHT;
};


V2P main(uint vertexID : SV_VertexID, uint instance : SV_InstanceID)
{
    StructuredBuffer<PointLight> lights = ResourceDescriptorHeap[resources.lightsIndex];
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[resources.spherePositionIndex];
   
    ConstantBuffer<Uniforms> uniforms = ResourceDescriptorHeap[resources.uniformsIndex];
    
    float3 position = positions[vertexID] * lights[instance].radius + lights[instance].position;

    V2P output;
    output.Position = mul(uniforms.ViewProjection, float4(position, 1));
    output.light = lights[instance];
    return output;
}