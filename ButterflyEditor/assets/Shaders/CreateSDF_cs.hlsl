
struct SDFTriangle
{
    float3 Vertex0;
    float3 Vertex1;
    float3 Vertex2;
};

struct SDFUniformData
{
    float3 MinBounds;
    float Padding1;
    float3 MaxBounds;
    float Padding2;
    uint3 Resolution;
    uint NumTriangles;
};

struct ButterIndices
{
    int sdfTextureIndex;
    int triangleBufferIndex;
    int boundsBufferIndex;
};

ConstantBuffer<ButterIndices> resources : register(b0);

float3 GetVoxelPosition(uint3 voxel, float3 min, float3 max, uint3 resolution)
{
    float3 size = max - min;
    float3 voxelSize = size / float3(resolution);
    return min + (float3(voxel) + 0.5) * voxelSize;
}

// Taken From ChatGPT: Distance from triangle hlsl implementation.
float DistanceToTriangle(float3 p, float3 a, float3 b, float3 c)
{
    float3 ab = b - a;
    float3 ac = c - a;
    float3 ap = p - a;

    float d1 = dot(ab, ap);
    float d2 = dot(ac, ap);

    // Closest to vertex A
    if (d1 <= 0.0 && d2 <= 0.0)
        return length(p - a);

    float3 bp = p - b;

    float d3 = dot(ab, bp);
    float d4 = dot(ac, bp);

    // Closest to vertex B
    if (d3 >= 0.0 && d4 <= d3)
        return length(p - b);

    // Closest to edge AB
    float vc = d1 * d4 - d3 * d2;

    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
    {
        float v = d1 / (d1 - d3);
        float3 closest = a + v * ab;

        return length(p - closest);
    }

    float3 cp = p - c;

    float d5 = dot(ab, cp);
    float d6 = dot(ac, cp);

    // Closest to vertex C
    if (d6 >= 0.0 && d5 <= d6)
        return length(p - c);

    // Closest to edge AC
    float vb = d5 * d2 - d1 * d6;

    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
    {
        float w = d2 / (d2 - d6);
        float3 closest = a + w * ac;

        return length(p - closest);
    }

    // Closest to edge BC
    float va = d3 * d6 - d5 * d4;

    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
    {
        float w = (d4 - d3) /
                  ((d4 - d3) + (d5 - d6));

        float3 closest = b + w * (c - b);

        return length(p - closest);
    }

    // Closest point is inside the triangle
    float3 normal = normalize(cross(ab, ac));

    return abs(dot(p - a, normal));
}

[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<SDFUniformData> uniformData = ResourceDescriptorHeap[resources.boundsBufferIndex];
    RWTexture3D<float> outTexture = ResourceDescriptorHeap[resources.sdfTextureIndex];
    StructuredBuffer<SDFTriangle> triangleBuffer = ResourceDescriptorHeap[resources.triangleBufferIndex];
    
    float3 voxelPosition = GetVoxelPosition(id, uniformData.MinBounds, uniformData.MaxBounds, uniformData.Resolution);
    
    
    float distance = 1e10;
    
    for (uint i = 0; i < uniformData.NumTriangles; ++i)
    {
        SDFTriangle tri = triangleBuffer[i];

        distance = min(distance, DistanceToTriangle(voxelPosition, tri.Vertex0, tri.Vertex1, tri.Vertex2));
    }
    outTexture[id] = distance;
}