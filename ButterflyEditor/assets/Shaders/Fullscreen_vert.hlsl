struct V2P
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
};

V2P main(uint vertexID : SV_VertexID)
{
    V2P output;

    static const float2 positions[6] =
    {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0),

        float2(-1.0, -1.0),
        float2(1.0, 1.0),
        float2(1.0, -1.0)
    };

    static const float2 uvs[6] =
    {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0),

        float2(0.0, 1.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0)
    };

    output.Position = float4(positions[vertexID], 1.0, 1.0);
    output.UV = uvs[vertexID];

    return output;
}