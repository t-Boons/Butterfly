#pragma once

#define PI 3.14159265359
#define EPSILON 0.0001
#define FLT_MAX 3.402823466e+38F
#define MAX_CASCADES 4
#define CUBEMAP_SLICES 6

#define INVALID -1

inline bool IsValid(int id) { return id >= 0; }

float3 GetDirection(uint face, float2 uv)
{
    uv = uv * 2.0f - 1.0f;
    switch (face)
    {
        case 0:  return normalize(float3( 1.0f, -uv.y, -uv.x)); // +X
        case 1:  return normalize(float3(-1.0f, -uv.y,  uv.x)); // -X
        case 2:  return normalize(float3( uv.x,  1.0f,  uv.y)); // +Y
        case 3:  return normalize(float3( uv.x, -1.0f, -uv.y)); // -Y
        case 4:  return normalize(float3( uv.x, -uv.y,  1.0f)); // +Z
        default: return normalize(float3(-uv.x, -uv.y, -1.0f)); // -Z
    }
}