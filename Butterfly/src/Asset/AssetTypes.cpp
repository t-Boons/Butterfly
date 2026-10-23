#include "Asset/AssetTypes.hpp"


namespace Butterfly
{
	void SDF::GenerateSDF(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices, const std::vector<MeshAsset::SubMesh>& subMeshes, const Bounds& meshBounds)
	{
		const glm::vec3 padding = m_sdfBounds.Size() * 0.5f * 0.2f; // 20% of the size;
		m_sdfBounds.Min -= padding;
		m_sdfBounds.Max += padding;

		m_sdfResolution = glm::vec3(64.0f, 64.0f, 64.0f);


		std::vector<SDFTriangle> triangles;
		triangles.reserve(indices.size() / 3);

		for (auto& subMesh : subMeshes)
		{
			for (uint32_t i = 0; i < subMesh.IndexCount; i += 3)
			{
				uint32_t index0 = indices[subMesh.IndexOffset + i];
				uint32_t index1 = indices[subMesh.IndexOffset + i + 1];
				uint32_t index2 = indices[subMesh.IndexOffset + i + 2];
				SDFTriangle triangle;
				triangle.Vertex0 = positions[index0];
				triangle.Vertex1 = positions[index1];
				triangle.Vertex2 = positions[index2];
				triangles.push_back(triangle);
			}
		}

		m_sdf = GraphicsCommands::CreateSDF(triangles, meshBounds, m_sdfResolution);
	}




	void MeshAsset::LoadToGPU()
	{
		if (m_gpuLoaded)
		{
			return;
		}

		GPUIndices = RefPtr<BFIndexBuffer>(new BFIndexBuffer(Indices.data(), static_cast<uint32_t>(Indices.size()), DXGI_FORMAT_R32_UINT, "Indices"));


		BFStructuredBufferDesc desc;
		desc.HeapType = BFHeapType::Default;


		{
			desc.Data = Positions.data();
			desc.NumElements = static_cast<uint32_t>(Positions.size());
			desc.Stride = sizeof(glm::vec3);
			desc.DebugName = "Positions";

			GPUPositions = MakeRef<BFStructuredBuffer>(desc);
		}

		{
			desc.Data = Normals.data();
			desc.NumElements = static_cast<uint32_t>(Normals.size());
			desc.Stride = sizeof(glm::vec3);
			desc.DebugName = "Normals";
			GPUNormals = MakeRef<BFStructuredBuffer>(desc);
		}
		{
			desc.Data = Tangents.data();
			desc.NumElements = static_cast<uint32_t>(Tangents.size());
			desc.Stride = sizeof(glm::vec4);
			desc.DebugName = "Tangents";
			GPUTangents = MakeRef<BFStructuredBuffer>(desc);
		}
		{
			desc.Data = UVs.data();
			desc.NumElements = static_cast<uint32_t>(UVs.size());
			desc.Stride = sizeof(glm::vec2);
			desc.DebugName = "UVs";
			GPUUVs = MakeRef<BFStructuredBuffer>(desc);
		}

		m_gpuLoaded = true;

		for (auto& subMesh : SubMeshes)
		{
			for (uint32_t i = 0; i < subMesh.IndexCount; ++i)
			{
				uint32_t index = Indices[subMesh.IndexOffset + i];

				const glm::vec3 position = Positions[index];
				if (m_meshBounds.Min.x > position.x) m_meshBounds.Min.x = position.x;
				if (m_meshBounds.Min.y > position.y) m_meshBounds.Min.y = position.y;
				if (m_meshBounds.Min.z > position.z) m_meshBounds.Min.z = position.z;
				if (m_meshBounds.Max.x < position.x) m_meshBounds.Max.x = position.x;
				if (m_meshBounds.Max.y < position.y) m_meshBounds.Max.y = position.y;
				if (m_meshBounds.Max.z < position.z) m_meshBounds.Max.z = position.z;
			}
		}
		const glm::vec3 padding = m_meshBounds.Size() * 0.5f * 0.2f; // 20% of the size;
		m_meshBounds.Min -= padding;
		m_meshBounds.Max += padding;

		if (CreateSDF)
		{
			m_sdf = MakeRef<SDF>();
			m_sdf->GenerateSDF(Positions, Indices, SubMeshes, m_meshBounds);
		}
	}
}