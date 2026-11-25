#pragma once

#include "Common.hlsli"
#include "Mesh.hlsli"
#include "Material.hlsli"
#include "Camera.hlsli"
#include "Shadow.hlsli"
#include "Model.hlsli"
#include "Light.hlsli"

struct FrameData
{
    int MeshBuffer;
    int NumMeshes;

    int ModelBuffer;
    int NumModels;

    int LightBuffer;
    int NumLights;

    int ShadowMapBuffer;
    int NumShadowMaps;

    int MaterialBuffer;
    int NumMaterials;

    int CameraConstantData;

    int GetNumMeshes() { return NumMeshes; }
    int GetNumModels() { return NumModels; }
    int GetNumLights() { return NumLights; }
    int GetNumShadowMaps() { return NumShadowMaps; }
    int GetNumMaterials() { return NumMaterials; }

    Mesh GetMesh(int index)
    {
        StructuredBuffer<Mesh> meshBuffer = ResourceDescriptorHeap[MeshBuffer];
        return meshBuffer[index];
    }

    ModelData GetModelData(int index)
    {
        StructuredBuffer<ModelData> modelDataBuffer = ResourceDescriptorHeap[ModelBuffer];
        return modelDataBuffer[index];
    }

    Light GetLight(int index)
    {
        StructuredBuffer<Light> lightBuffer = ResourceDescriptorHeap[LightBuffer];
        return lightBuffer[index];
    }

    ShadowMap GetShadowMap(int index)
    {
        StructuredBuffer<ShadowMap> shadowMapBuffer = ResourceDescriptorHeap[ShadowMapBuffer];
        return shadowMapBuffer[index];
    }

    MaterialData GetMaterial(int index)
    {
        StructuredBuffer<MaterialData> materialBuffer = ResourceDescriptorHeap[MaterialBuffer];
        return materialBuffer[index];
    }

    CameraData GetCameraData()
    {
        ConstantBuffer<CameraData> cameraData = ResourceDescriptorHeap[CameraConstantData];
        return cameraData;
    }
};

FrameData GetFrameData(int frameDataIndex)
{
    ConstantBuffer<FrameData> frameData = ResourceDescriptorHeap[frameDataIndex];
    return frameData;
}