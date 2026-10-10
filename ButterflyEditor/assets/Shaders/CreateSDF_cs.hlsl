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

static const float PI = 3.14159265358979323846;

float3 GetVoxelPosition(uint3 voxel, float3 minB, float3 maxB, uint3 resolution)
{
    float3 voxelSize = (maxB - minB) / float3(resolution);
    return minB + (float3(voxel) + 0.5) * voxelSize;
}

float SegmentDistanceSquared(float3 p, float3 a, float3 b)
{
    float3 ab = b - a;
    float lenSq = dot(ab, ab);
    if (lenSq < 1e-20)
        return dot(p - a, p - a);

    float t = saturate(dot(p - a, ab) / lenSq);
    float3 q = a + t * ab;
    return dot(p - q, p - q);
}

float DistanceToTriangleSquared(float3 p, float3 a, float3 b, float3 c)
{
    float3 ab = b - a;
    float3 ac = c - a;
    float3 ap = p - a;

    float3 n = cross(ab, ac);
    if (dot(n, n) <= 1e-10 * dot(ab, ab) * dot(ac, ac))
    {
        return min(SegmentDistanceSquared(p, a, b),
               min(SegmentDistanceSquared(p, b, c),
                   SegmentDistanceSquared(p, c, a)));
    }

    float d1 = dot(ab, ap);
    float d2 = dot(ac, ap);
    if (d1 <= 0.0 && d2 <= 0.0)
        return dot(ap, ap);

    float3 bp = p - b;
    float d3 = dot(ab, bp);
    float d4 = dot(ac, bp);
    if (d3 >= 0.0 && d4 <= d3)
        return dot(bp, bp);

    float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
    {
        float v = d1 / (d1 - d3);
        float3 q = a + v * ab;
        return dot(p - q, p - q);
    }

    float3 cp = p - c;
    float d5 = dot(ab, cp);
    float d6 = dot(ac, cp);
    if (d6 >= 0.0 && d5 <= d6)
        return dot(cp, cp);

    float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
    {
        float w = d2 / (d2 - d6);
        float3 q = a + w * ac;
        return dot(p - q, p - q);
    }

    float va = d3 * d6 - d5 * d4;
    if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0)
    {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        float3 q = b + w * (c - b);
        return dot(p - q, p - q);
    }

    float denom = 1.0 / (va + vb + vc);
    float3 q = a + ab * (vb * denom) + ac * (vc * denom);
    return dot(p - q, p - q);
}


float TriangleWinding(float3 p, float3 a, float3 b, float3 c)
{
    float3 A = a - p;
    float3 B = b - p;
    float3 C = c - p;

    float la = length(A);
    float lb = length(B);
    float lc = length(C);

    if (min(la, min(lb, lc)) < 1e-9)
        return 0.0;

    float num = dot(A, cross(B, C));
    float den = la * lb * lc
              + dot(A, B) * lc
              + dot(B, C) * la
              + dot(C, A) * lb;

    return 2.0 * atan2(num, den) / (4.0 * PI);
}

// Shader was created using Claude.
// It computes the signed distance field (SDF) of a 3D model represented by triangles.
[numthreads(8, 8, 8)]
void main(uint3 id : SV_DispatchThreadID)
{
    ConstantBuffer<SDFUniformData> uniformData = ResourceDescriptorHeap[resources.boundsBufferIndex];
    RWTexture3D<float> outTexture = ResourceDescriptorHeap[resources.sdfTextureIndex];
    StructuredBuffer<SDFTriangle> triangleBuffer = ResourceDescriptorHeap[resources.triangleBufferIndex];

    if (any(id >= uniformData.Resolution))
        return;

    float3 p = GetVoxelPosition(id, uniformData.MinBounds, uniformData.MaxBounds, uniformData.Resolution);

    float minDistSq = 3.402823466e+38;
    float winding = 0.0;

    for (uint i = 0; i < uniformData.NumTriangles; ++i)
    {
        SDFTriangle tri = triangleBuffer[i];

        minDistSq = min(minDistSq, DistanceToTriangleSquared(p, tri.Vertex0, tri.Vertex1, tri.Vertex2));
        winding += TriangleWinding(p, tri.Vertex0, tri.Vertex1, tri.Vertex2);
    }

    float distance = sqrt(minDistSq);
    const float softness = 0.15;
    float s = clamp((0.5 - abs(winding)) / softness, -1.0, 1.0);

    outTexture[id] = s * distance;
}