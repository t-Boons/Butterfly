#pragma once
#include "Core/Common.hpp"
#include "glm/glm.hpp"

namespace Butterfly
{
	class ModelImporter
	{
	public:
		struct Texture
		{
			std::string m_name;
			int m_width;
			int m_height;
			int m_channels;
			int m_bits;
			std::vector<uint8_t> m_image;
		};

		struct Material
		{
			float m_metallic = 0.5f;
			float m_roughness = 0.5f;
			glm::vec4 m_baseColor = glm::vec4(1.0f);
			glm::vec4 m_emissiveColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			RefPtr<Texture> m_colorTexture;
			RefPtr<Texture> m_normalTexture;
			RefPtr<Texture> m_metallicRoughnessTexture;
			RefPtr<Texture> m_emissionTexture;
			RefPtr<Texture> m_ambientOcclusionTexture;
			float m_normalScale = 1.0f;

			std::string m_name;
		};

		struct Mesh
		{
			std::vector<std::vector<uint32_t>> m_indices;
			std::vector<std::vector<glm::vec3>> m_positions;
			std::vector<std::vector<glm::vec3>> m_normals;
			std::vector<std::vector<glm::vec2>> m_texcoords;
			std::vector<std::vector<glm::vec4>> m_tangents;
			std::vector<std::vector<glm::vec3>> m_bitangents;

			std::string m_name;
		};

		struct Model
		{
			int m_meshIndex = -1;
			std::vector<uint32_t> m_materialIndices;

			std::string m_filePath;
		};

		struct Node
		{
			std::vector<RefPtr<Node>> m_children;

			glm::mat4 m_matrix = glm::mat4(1.0f);
			size_t depth = 0;
			RefPtr<Model> m_model;
			std::string n_name;
		};

		static RefPtr<ModelImporter> Create(const std::string& filePath);

		void Load() { LoadImpl(m_filePath); }

		bool Parsed() const { return m_root != nullptr; }
		const std::vector<RefPtr<ModelImporter::Material>>& Materials() const { return m_materials; }
		const std::vector<RefPtr<ModelImporter::Mesh>>& Meshes() const { return m_meshes; }
		const std::vector<RefPtr<ModelImporter::Model>>& Models() const { return m_models; }
		std::string FilePath() const { return m_filePath; }
		const RefPtr<Node> Root() const { return m_root; }

		virtual ~ModelImporter() {};

	protected:
		virtual void LoadImpl(const std::string& filePath) = 0;

		std::vector<RefPtr<ModelImporter::Material>> m_materials;
		std::vector<RefPtr<ModelImporter::Mesh>> m_meshes;
		std::vector<RefPtr<ModelImporter::Model>> m_models;
		std::string m_filePath;
		RefPtr<Node> m_root;
	};

}