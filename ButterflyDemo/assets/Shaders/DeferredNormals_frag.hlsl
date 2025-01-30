struct V2P
{
    float4 position : SV_Position;
    float3 normals : NORMAL0;
};

float4 main(V2P pixelInput) : SV_TARGET
{
    return float4(pixelInput.normals.xyz, 1.0f + -pixelInput.position.z);
}