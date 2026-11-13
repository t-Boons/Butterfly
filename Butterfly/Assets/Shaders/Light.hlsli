#pragma once

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