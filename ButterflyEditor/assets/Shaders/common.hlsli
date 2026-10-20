#define PI 3.14159265359
#define EPSILON 0.0001
#define FLT_MAX 3.402823466e+38F

struct CameraData
{
    float4x4 ViewProj;
    float4x4 InvProj;
    float4x4 InvView;
    float3 Position;
    float Near;
    float Far;
};

struct ModelData
{
    float4x4 ModelMatrix;
    float4x4 InverseModelMatrix;
    float4x4 NormalMatrix;
    float4 BoundsMin;
    float4 BoundsSize;
    float4 SDFResolution;
    int SDFTextureIndex;
};