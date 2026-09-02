#pragma once
#include "Renderer/Buffer_DX12.hpp"
#include "Profiler.hpp"

namespace Butterfly
{
	class FullscreenQuad
	{
    public:
        struct Vertex {
            float x, y, z;
            float u, v;
        };

        static BFStructuredBuffer* GetVertexBuffer()
        {
            BF_PROFILE_EVENT();

            if (!s_vertBuffer)
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.Buffer.FirstElement = 0;
                srvDesc.Buffer.NumElements = sizeof(s_vertices) / sizeof(Vertex);
                srvDesc.Buffer.StructureByteStride = sizeof(Vertex);
                srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                s_vertBuffer = new BFStructuredBuffer(&s_vertices[0], static_cast<uint32_t>(sizeof(s_vertices)), &srvDesc, "FullscreenQuad Vertexbuffer");
            }
            return s_vertBuffer;
        }

        static BFIndexBuffer* GetIndexBuffer()
        {
            BF_PROFILE_EVENT();

            if (!s_indexBuffer)
            {
                s_indexBuffer = new BFIndexBuffer(&s_indices[0], static_cast<uint32_t>(sizeof(s_indices)), DXGI_FORMAT_R32_UINT, "FullscreenQuad Indexbuffer");
            }
            return s_indexBuffer;
        }

        static void ShutDown()
        {
            FREE(s_vertBuffer);
            FREE(s_indexBuffer);
        }

	private:
        inline static BFStructuredBuffer* s_vertBuffer;
        inline static BFIndexBuffer* s_indexBuffer;

        inline static FullscreenQuad::Vertex s_vertices[4] = {
            { -1.0f,  1.0f, 0.0f, 0.0f, 0.0f },
            {  1.0f,  1.0f, 0.0f, 1.0f, 0.0f },
            { -1.0f, -1.0f, 0.0f, 0.0f, 1.0f },
            {  1.0f, -1.0f, 0.0f, 1.0f, 1.0f },
        };

        inline static uint32_t s_indices[6] = { 0, 1, 2, 1, 3, 2 };
	};
}