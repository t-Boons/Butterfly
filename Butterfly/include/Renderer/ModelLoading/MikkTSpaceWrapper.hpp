#pragma once
#include "Core/Common.hpp"

BF_WARNING_PUSH(0)
#include "MikkTSpace/mikktspace.h"
BF_WARNING_POP
#include "glm/glm.hpp"

namespace Butterfly
{
	class MikkTSpaceTangent
	{
	public:
		struct MikktSpaceMesh
		{
			std::vector<uint32_t>* m_indices = nullptr;
			std::vector<glm::vec3>* m_positions = nullptr;
			std::vector<glm::vec3>* m_normals = nullptr;
			std::vector<glm::vec2>* m_texcoords = nullptr;
		};

		static bool GetTangents(const MikktSpaceMesh& mesh, std::vector<glm::vec4>& tangents);

	private:
		struct MikkiTGltfContext
		{
			const MikktSpaceMesh* m_mesh;
			std::vector<glm::vec4> m_outTangents;
		};

		static int GetNumFaces(const SMikkTSpaceContext* context);

		static int GetNumVerticesOfFace(const SMikkTSpaceContext*, const int);

		static void GetPosition(const SMikkTSpaceContext* context, float posOut[3], const int faceIdx, const int vertIdx);

		static void GetNormal(const SMikkTSpaceContext* context, float normOut[3], const int faceIdx, const int vertIdx);

		static void GetTexCoord(const SMikkTSpaceContext* context, float uvOut[2], const int faceIdx, const int vertIdx);

		static void SetTangent(const SMikkTSpaceContext* context,
			const float tangent[3],
			const float bitangentSign,
			const int faceIdx,
			const int vertIdx);
	};
}  // namespace bee