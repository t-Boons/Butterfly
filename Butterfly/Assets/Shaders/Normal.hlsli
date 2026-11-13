#pragma once

float3 MapNormal(float3 sampledNormal, float3 vertexNormal, float3 vertexTangent, float tangentSign, float normalScale)
{
    float3 N = normalize(vertexNormal);

    float3 T = normalize(vertexTangent);
    T = normalize(T - N * dot(N, T));

    float3 B = cross(N, T) * tangentSign;

    float3 tangentNormal = sampledNormal * 2.0 - 1.0;
    tangentNormal.xy *= normalScale;

    return normalize(
        T * tangentNormal.x +
        B * tangentNormal.y +
        N * tangentNormal.z
    );
}