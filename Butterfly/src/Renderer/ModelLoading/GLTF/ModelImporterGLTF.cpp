#include "glm/gtx/quaternion.hpp"
#include "Renderer/ModelLoading/GLTF/ModelImporterGLTF.hpp"
#include "Renderer/ModelLoading/MikkTSpaceWrapper.hpp"

#include "tinygltf/stb_image_write.h"
#include "tinygltf/stb_image.h"
#include "tinygltf/tiny_gltf.h"

namespace Butterfly
{
	std::vector<uint32_t> LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
	{
		const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
		const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

		std::vector<uint32_t> outIndices;
		const void* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];

		// Just copy the vector if the index data type is uint32_t (we prefer this)
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			auto indexArray = reinterpret_cast<const uint32_t*>(dataPtr);
			outIndices.assign(indexArray, indexArray + accessor.count);
		}

		// Change it from a ushort to a uint32_t instead.
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			auto indexArray = reinterpret_cast<const uint16_t*>(dataPtr);

			const uint32_t size = static_cast<uint32_t>(accessor.count);

			outIndices.reserve(size);
			for (size_t i = 0; i < size; i++)
			{
				outIndices.push_back((static_cast<uint32_t>(indexArray[i])));
			}
		}

		return outIndices;
	}

	template <typename DataType>
	std::vector<DataType> LoadAttribute(const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		char const* attributeName)
	{
		if (primitive.attributes.find(attributeName) == primitive.attributes.end()) return std::vector<DataType>();

		const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find(attributeName)->second];
		const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

		auto indexArray = reinterpret_cast<const DataType*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

		std::vector<DataType> outArr;
		outArr.assign(indexArray, indexArray + accessor.count);
		return outArr;
	}

	void LoadGltfModel(const std::string& filePath, tinygltf::Model& model)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool success = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

		Log::Assert(success, "%s $s", err.c_str(), filePath.c_str());

		if (!warn.empty())
		{
			Log::Warn("%s %s", warn.c_str(), filePath.c_str());
		}
	}

	std::vector<glm::vec3> CalculateBITangents(const std::vector<glm::vec4>& tangents, const std::vector<glm::vec3>& normals)
	{
		const size_t arraySize = tangents.size();
		if (arraySize != normals.size()) return std::vector<glm::vec3>();

		std::vector<glm::vec3> bitangents;
		bitangents.resize(arraySize);

		for (size_t i = 0; i < arraySize; ++i)
		{
			const glm::vec3 tangentxyz = { tangents[i].x, tangents[i].y, tangents[i].z };
			bitangents[i] = glm::cross(normals[i], tangentxyz) * tangents[i].w;
		}

		return bitangents;
	}

	RefPtr<ModelImporter::Texture> LoadTextureFromMaterial(const tinygltf::Model& model,
		const tinygltf::Material& material,
		const std::string& attribName)
	{
		const bool foundValue = material.values.find(attribName) != material.values.end();
		const bool foundAdditionalValue = material.additionalValues.find(attribName) != material.additionalValues.end();

		if (!foundValue && !foundAdditionalValue) return nullptr;

		int index = -1;
		if (foundValue)
		{
			index = material.values.at(attribName).TextureIndex();
		}
		else
		{
			index = material.additionalValues.at(attribName).TextureIndex();
		}

		const tinygltf::Texture& tex = model.textures[index];
		const int imgIndex = tex.source;
		const auto& img = model.images[imgIndex];

		RefPtr<ModelImporter::Texture> outTexture = MakeRef<ModelImporter::Texture>();
		outTexture->m_image = img.image;
		outTexture->m_width = img.width;
		outTexture->m_height = img.height;
		outTexture->m_channels = img.component;
		outTexture->m_name = img.name;
		return outTexture;
	}

	RefPtr<ModelImporter::Material> LoadMaterialFromGltfMaterial(const tinygltf::Model& model, const tinygltf::Material& material)
	{
		RefPtr<ModelImporter::Material> outMaterial = MakeRef<ModelImporter::Material>();
		outMaterial->m_colorTexture = LoadTextureFromMaterial(model, material, "baseColorTexture");
		outMaterial->m_metallicRoughnessTexture = LoadTextureFromMaterial(model, material, "metallicRoughnessTexture");
		outMaterial->m_normalTexture = LoadTextureFromMaterial(model, material, "normalTexture");
		outMaterial->m_emissionTexture = LoadTextureFromMaterial(model, material, "emissiveTexture");
		outMaterial->m_ambientOcclusionTexture = LoadTextureFromMaterial(model, material, "occlusionTexture");
		outMaterial->m_name = material.name;

		const uint32_t numCV = static_cast<uint32_t>(material.pbrMetallicRoughness.baseColorFactor.size());
		for (uint32_t i = 0; i < numCV; i++)
		{
			outMaterial->m_baseColor[i] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[i]);
		}

		const uint32_t numEC = static_cast<uint32_t>(material.emissiveFactor.size());
		for (uint32_t i = 0; i < numEC; i++)
		{
			outMaterial->m_emissiveColor[i] = static_cast<float>(material.emissiveFactor[i]);
		}

		outMaterial->m_metallic = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
		outMaterial->m_roughness = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
		outMaterial->m_normalScale = static_cast<float>(material.normalTexture.scale);

		return outMaterial;
	}

	RefPtr<ModelImporter::Mesh> LoadMeshFromGltfMesh(const tinygltf::Model& model, const std::vector<tinygltf::Primitive>& primitives)
	{
		RefPtr<ModelImporter::Mesh> outMesh = MakeRef<ModelImporter::Mesh>();

		for (auto& prim : primitives)
		{
			outMesh->m_indices.push_back(LoadIndices(model, prim));
			outMesh->m_positions.push_back(LoadAttribute<glm::vec3>(model, prim, "POSITION"));
			outMesh->m_normals.push_back(LoadAttribute<glm::vec3>(model, prim, "NORMAL"));
			outMesh->m_texcoords.push_back(LoadAttribute<glm::vec2>(model, prim, "TEXCOORD_0"));
			outMesh->m_tangents.push_back(LoadAttribute<glm::vec4>(model, prim, "TANGENT"));
		}
		for (uint32_t i = 0; i < static_cast<uint32_t>(outMesh->m_tangents.size()); ++i)
		{
			if (outMesh->m_tangents[i].empty())
			{
				MikkTSpaceTangent::MikktSpaceMesh m;
				m.m_indices = &outMesh->m_indices[i];
				m.m_normals = &outMesh->m_normals[i];
				m.m_positions = &outMesh->m_positions[i];
				m.m_texcoords = &outMesh->m_texcoords[i];
				MikkTSpaceTangent::GetTangents(m, outMesh->m_tangents[i]);
			}

			outMesh->m_bitangents.push_back(CalculateBITangents(outMesh->m_tangents[i], outMesh->m_normals[i]));
		}

		return outMesh;
	}

	std::vector<RefPtr<ModelImporter::Material>> LoadMaterials(const tinygltf::Model& model)
	{
		std::vector<RefPtr<ModelImporter::Material>> materials;
		const size_t count = model.materials.size();
		if (count == 0) return materials;
		materials.resize(count);

		for (size_t i = 0; i < count; i++)
		{
			materials[i] = LoadMaterialFromGltfMaterial(model, model.materials[i]);
		}

		return materials;
	}

	std::vector<RefPtr<ModelImporter::Mesh>> LoadMeshes(const tinygltf::Model& model)
	{
		std::vector<RefPtr<ModelImporter::Mesh>> meshes;
		const size_t count = model.meshes.size();
		if (count == 0) return meshes;
		meshes.resize(count);

		for (size_t i = 0; i < count; i++)
		{
			meshes[i] = LoadMeshFromGltfMesh(model, model.meshes[i].primitives);
			meshes[i]->m_name = model.meshes[i].name;
		}

		return meshes;
	}

	std::vector<RefPtr<ModelImporter::Node>> TraverseNode(const tinygltf::Model& gltfModel,
		const std::vector<int>& childrenIndices,
		int depth,
		const std::vector<RefPtr<ModelImporter::Model>>& models)
	{
		std::vector<RefPtr<ModelImporter::Node>> outNodes;
		outNodes.resize(childrenIndices.size());

		int childIndex = 0;
		for (int nodeIndex : childrenIndices)
		{
			const tinygltf::Node& gltfNode = gltfModel.nodes[nodeIndex];
			RefPtr<ModelImporter::Node> newNode = MakeRef<ModelImporter::Node>();

			newNode->n_name = gltfNode.name;

			auto translation = glm::vec3(0.0f);
			auto rotation = glm::identity<glm::quat>();
			auto scale = glm::vec3(1.0f);

			for (uint32_t i = 0; i < static_cast<uint32_t>(gltfNode.translation.size()); ++i) translation[i] = static_cast<float>(gltfNode.translation[i]);
			for (uint32_t i = 0; i < static_cast<uint32_t>(gltfNode.rotation.size()); ++i) rotation[i] = static_cast<float>(gltfNode.rotation[static_cast<uint32_t>(gltfNode.rotation.size()) - i - 1]);
			for (uint32_t i = 0; i < static_cast<uint32_t>(gltfNode.scale.size()); ++i) scale[i] = static_cast<float>(gltfNode.scale[i]);

			newNode->m_matrix =
				glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);

			if (gltfNode.mesh >= 0) newNode->m_model = models[gltfNode.mesh];
			newNode->depth = depth;
			newNode->m_children = TraverseNode(gltfModel, gltfNode.children, depth + 1, models);

			outNodes[childIndex] = newNode;
			childIndex++;
		}

		return outNodes;
	}

	RefPtr<ModelImporter::Node> CreateNodes(const tinygltf::Model& gltfModel,
		const std::vector<RefPtr<ModelImporter::Model>>& models)
	{
		RefPtr<ModelImporter::Node> root = MakeRef<ModelImporter::Node>();
		root->m_children = TraverseNode(gltfModel, gltfModel.scenes[gltfModel.defaultScene].nodes, 1, models);
		return root;
	}

	std::vector<RefPtr<ModelImporter::Model>> LoadModels(const tinygltf::Model& model)
	{
		std::vector<RefPtr<ModelImporter::Model>> outModels;
		outModels.resize(model.meshes.size());

		int it = 0;
		for (const auto& mesh : model.meshes)
		{
			auto newModel = MakeRef<ModelImporter::Model>();

			for (const auto& prim : mesh.primitives)
			{
				newModel->m_materialIndices.push_back(prim.material);
			}
			newModel->m_meshIndex = it;

			outModels[it] = newModel;
			it++;
		}

		return outModels;
	}

	void ModelImporterGltf::LoadImpl(const std::string& filePath)
	{
		tinygltf::Model gltfModel;
		LoadGltfModel(filePath, gltfModel);

		m_materials = LoadMaterials(gltfModel);
		m_meshes = LoadMeshes(gltfModel);
		m_models = LoadModels(gltfModel);
		m_root = CreateNodes(gltfModel, m_models);
	}
}