#pragma once

struct CameraData
{
    float4x4 ViewProj;
    float4x4 InvProj;
    float4x4 InvView;
    float3 Position;
    float Near;
    float Far;
};