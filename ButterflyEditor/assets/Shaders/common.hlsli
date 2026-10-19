#define PI 3.14159265359
#define EPSILON 0.0001
#define FLT_MAX 3.402823466e+38F

struct CameraData
{
    float4x4 ViewProj;
    float3 Position;
    float4x4 InvView;
    float4x4 InvProj;
};