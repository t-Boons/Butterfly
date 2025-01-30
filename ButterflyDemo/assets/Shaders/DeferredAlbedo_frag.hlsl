

struct V2P
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD0;
    nointerpolation int samplerIndex : SAMPLERINDEX;
    nointerpolation int textureIndex : TEXTUREINDEX;
};

float4 main(V2P pixelInput) : SV_TARGET
{
    SamplerState smp = SamplerDescriptorHeap[pixelInput.samplerIndex];
    Texture2D<float4> tex = ResourceDescriptorHeap[pixelInput.textureIndex];
    
    return tex.Sample(smp, pixelInput.texCoord);
}