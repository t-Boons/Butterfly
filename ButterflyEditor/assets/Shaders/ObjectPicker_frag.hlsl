struct V2P
{
    float4 position : SV_Position;
    uint color : COLOR0;
};


uint main(V2P pixelInput) : SV_TARGET0
{
    return pixelInput.color;
}