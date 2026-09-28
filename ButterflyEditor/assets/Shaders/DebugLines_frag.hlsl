struct V2P
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};


float4 main(V2P pixelInput) : SV_TARGET0
{
    return pixelInput.color;

}