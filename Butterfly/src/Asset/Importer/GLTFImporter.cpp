#include "Asset/Importer/GLTFImporter.hpp"
#include "Asset/Importer/ImageImporter.hpp"
#include "Asset/AssetTypes.hpp"
#include "Renderer/D3D12Texture.hpp"
#include "Renderer/ModelLoading/MikkTSpaceWrapper.hpp"
#include "tinygltf/tiny_gltf.h"
#include "Core/Application.hpp"

namespace Butterfly
{
	BF_REGISTER_ASSET_IMPORTER(GLTFImporter);

	
	struct LoadedIndices
	{
		std::vector<uint32_t> Indices;
		std::vector<uint32_t> Offsets;
	};

	LoadedIndices LoadIndices(const tinygltf::Model& model, const std::vector<tinygltf::Primitive>& primitives, const std::vector<uint32_t>& vertexOffsets)
	{
		LoadedIndices result;

		uint32_t numIndices = 0;

		for (const auto& primitive : primitives)
		{
			if (primitive.indices < 0)
			{
				continue;
			}

			const auto& accessor = model.accessors[primitive.indices];
			numIndices += static_cast<uint32_t>(accessor.count);
		}

		result.Indices.resize(numIndices);
		result.Offsets.reserve(primitives.size());

		uint32_t startIndex = 0;

		for (size_t i = 0; i < primitives.size(); i++)
		{
			const auto& primitive = primitives[i];

			if (primitive.indices < 0)
			{
				continue;
			}

			result.Offsets.push_back(startIndex);

			const auto& accessor = model.accessors[primitive.indices];
			const auto& bufferView = model.bufferViews[accessor.bufferView];
			const auto& buffer = model.buffers[bufferView.buffer];

			const void* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
			const uint32_t vertexOffset = vertexOffsets[i];

			if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				const uint32_t* indexArray = reinterpret_cast<const uint32_t*>(dataPtr);

				for (size_t j = 0; j < accessor.count; j++)
				{
					result.Indices[startIndex++] = indexArray[j] + vertexOffset;
				}
			}
			else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				const uint16_t* indexArray = reinterpret_cast<const uint16_t*>(dataPtr);

				for (size_t j = 0; j < accessor.count; j++)
				{
					result.Indices[startIndex++] = static_cast<uint32_t>(indexArray[j]) + vertexOffset;
				}
			}
		}

		BF_CORE_ASSERT(startIndex != 0, "No indices were loaded from the GLTF model");
		BF_CORE_ASSERT(startIndex == numIndices, "Index buffer size mismatch");

		return result;
	}

	template <typename DataType>
	struct LoadedAttribute
	{
		std::vector<DataType> Data;
		std::vector<uint32_t> Offsets;
	};

	template <typename DataType>
	LoadedAttribute<DataType> LoadAttribute(const tinygltf::Model& model, const std::vector<tinygltf::Primitive>& primitives, char const* attributeName)
	{
		uint32_t numAttributes = 0;

		for (const auto& primitive : primitives)
		{
			auto it = primitive.attributes.find(attributeName);

			if (it == primitive.attributes.end())
			{
				continue;
			}

			numAttributes += static_cast<uint32_t>(model.accessors[it->second].count);
		}

		LoadedAttribute<DataType> result;
		result.Data.resize(numAttributes);
		result.Offsets.reserve(primitives.size());

		uint32_t startIndex = 0;

		for (const auto& primitive : primitives)
		{
			auto it = primitive.attributes.find(attributeName);

			if (it == primitive.attributes.end())
				return {};

			result.Offsets.push_back(startIndex);

			const tinygltf::Accessor& accessor = model.accessors[it->second];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			const DataType* indexArray = reinterpret_cast<const DataType*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

			std::copy(indexArray, indexArray + accessor.count, result.Data.begin() + startIndex);
			startIndex += static_cast<uint32_t>(accessor.count);
		}

		BF_CORE_ASSERT(startIndex != 0, "No attributes were loaded from the GLTF model: %s", attributeName);
		BF_CORE_ASSERT(startIndex == numAttributes, "Attribute buffer size mismatch: %s", attributeName);

		return result;
	}

	void LoadGltfModel(const std::string& filePath, tinygltf::Model& model)
	{
		tinygltf::TinyGLTF loader;
		std::string err;
		std::string warn;

		bool success = loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

		BF_CORE_ASSERT(success, "%s %s", err.c_str(), filePath.c_str());

		if (!warn.empty())
		{
			BF_CORE_LOG_WARN("%s %s", warn.c_str(), filePath.c_str());
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

	AssetHandle<TextureAsset> LoadTextureFromMaterial(const tinygltf::Model& model, const tinygltf::Material& material, const std::string& attribName, AssetManager& manager, const AssetFileMetadata& meta)
	{
		const bool foundValue = material.values.find(attribName) != material.values.end();
		const bool foundAdditionalValue = material.additionalValues.find(attribName) != material.additionalValues.end();

		if (!foundValue && !foundAdditionalValue) return {};

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


		BFTextureDesc desc;
		desc.Data = static_cast<const void*>(img.image.data());
		desc.Width = img.width;
		desc.Height = img.height;
		desc.Flags = BFTextureDesc::ShaderResource;
		desc.DebugName = img.name;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

		RefPtr<TextureAsset> outAsset = MakeRef<TextureAsset>();
		outAsset->Texture = BFTexture::CreateTextureFromCPUBuffer(desc);

		std::string name = (tex.name + "_" + attribName);
		if (tex.name.empty())
		{
			name = "Texture_" + std::to_string(index) + "_" + attribName;
		}
		const auto subMetaData = std::find_if(meta.Assets.begin(), meta.Assets.end(), [&](const auto& subMeta)
			{
				return subMeta.second.Name == name && subMeta.second.Type.TypeName == "Texture";
			});	

		if (subMetaData == meta.Assets.end())
		{
			BF_CORE_LOG_ERROR("Failed to find sub asset metadata for material: %s", tex.name.c_str());
			return {};
		}

		return manager.AddAssetEntry<TextureAsset>(AssetEntry{ subMetaData->second.AssetID, subMetaData->second.Type, outAsset });
	}

	bool LoadTextureMetadataFromGltfMaterial(const tinygltf::Model& model, const tinygltf::Material& material, const std::string& attribName, std::unordered_map<UUID, AssetMetadata>& metadata)
	{
		const bool foundValue = material.values.find(attribName) != material.values.end();
		const bool foundAdditionalValue = material.additionalValues.find(attribName) != material.additionalValues.end();

		if (!foundValue && !foundAdditionalValue)
		{
			return false;
		}

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

		AssetMetadata newMeta;
		newMeta.Name = tex.name + "_" + attribName;
		if (tex.name.empty())
		{
			newMeta.Name = "Texture_" + std::to_string(index) + "_" + attribName;
		}
		newMeta.Type = { "Texture" };
		newMeta.AssetID = UUID::Generate();
		metadata.insert({ newMeta.AssetID, newMeta });
		return true;
	}

	std::vector<AssetHandle<MaterialAsset>> LoadMaterials(const tinygltf::Model& model, AssetManager& manager, const AssetFileMetadata& meta)
	{
		std::vector<AssetHandle<MaterialAsset>> materials;
		const size_t count = model.materials.size();
		if (count == 0) return materials;
		materials.reserve(count);

		for (size_t i = 0; i < count; i++)
		{
			const tinygltf::Material& material = model.materials[i];
			RefPtr<MaterialAsset> outMaterial = MakeRef<MaterialAsset>();
			outMaterial->ColorTexture = LoadTextureFromMaterial(model, material, "baseColorTexture", manager, meta);
			outMaterial->MetallicRoughnessTexture = LoadTextureFromMaterial(model, material, "metallicRoughnessTexture", manager, meta);
			outMaterial->NormalTexture = LoadTextureFromMaterial(model, material, "normalTexture", manager, meta);
			outMaterial->EmissionTexture = LoadTextureFromMaterial(model, material, "emissiveTexture", manager, meta);
			outMaterial->AmbientOcclusionTexture = LoadTextureFromMaterial(model, material, "occlusionTexture", manager, meta);
			outMaterial->Name = material.name;

			const uint32_t numCV = static_cast<uint32_t>(material.pbrMetallicRoughness.baseColorFactor.size());
			for (uint32_t i = 0; i < numCV; i++)
			{
				outMaterial->BaseColor[i] = static_cast<float>(material.pbrMetallicRoughness.baseColorFactor[i]);
			}

			const uint32_t numEC = static_cast<uint32_t>(material.emissiveFactor.size());
			for (uint32_t i = 0; i < numEC; i++)
			{
				outMaterial->EmissiveColor[i] = static_cast<float>(material.emissiveFactor[i]);
			}

			outMaterial->Metallic = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
			outMaterial->Roughness = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);
			outMaterial->NormalScale = static_cast<float>(material.normalTexture.scale);


			std::string name = material.name;
			if (material.name.empty())
			{
				name = "Material_" + std::to_string(i);
			}

			const auto subMetaData = std::find_if(meta.Assets.begin(), meta.Assets.end(), [&](const auto& subMeta)
				{
					return subMeta.second.Name == name && subMeta.second.Type.TypeName == "Material";
				});

			if (subMetaData == meta.Assets.end())
			{
				BF_CORE_LOG_ERROR("Failed to find sub asset metadata for material: %s", material.name.c_str());
				return {};
			}

			materials.push_back(manager.AddAssetEntry<MaterialAsset>(AssetEntry{ subMetaData->second.AssetID, subMetaData->second.Type, outMaterial }));
		}

		return materials;
	}

	void LoadMaterialMetadata(const tinygltf::Model& model, std::unordered_map<UUID, AssetMetadata>& metadata)
	{
		for (uint32_t i = 0; i < model.materials.size(); i++)
		{
			AssetMetadata matMetadata;
			matMetadata.Name = model.materials[i].name;
			if (model.materials[i].name.empty())
			{
				matMetadata.Name = "Material_" + std::to_string(i);
			}
			matMetadata.Type = { "Material" };
			matMetadata.AssetID = UUID::Generate();

			metadata.insert({ matMetadata.AssetID, matMetadata });
			LoadTextureMetadataFromGltfMaterial(model, model.materials[i], "baseColorTexture", metadata);
			LoadTextureMetadataFromGltfMaterial(model, model.materials[i], "metallicRoughnessTexture", metadata);
			LoadTextureMetadataFromGltfMaterial(model, model.materials[i], "normalTexture", metadata);
			LoadTextureMetadataFromGltfMaterial(model, model.materials[i], "emissiveTexture", metadata);
			LoadTextureMetadataFromGltfMaterial(model, model.materials[i], "occlusionTexture", metadata);
		}
	}

	std::vector<AssetHandle<MeshAsset>> LoadMeshes(const tinygltf::Model& model, const std::vector<AssetHandle<MaterialAsset>>& materials, AssetManager& manager, const AssetFileMetadata& meta)
	{
		std::vector<AssetHandle<MeshAsset>> meshes;
		const size_t count = model.meshes.size();
		if (count == 0) return meshes;
		meshes.reserve(count);

		for (size_t i = 0; i < count; i++)
		{
			const tinygltf::Mesh& mesh = model.meshes[i];

			RefPtr<MeshAsset> outMesh = MakeRef<MeshAsset>();
			outMesh->Name = mesh.name;

			// Load attributes.
			const LoadedAttribute<glm::vec3> positions = LoadAttribute<glm::vec3>(model, mesh.primitives, "POSITION");
			const LoadedAttribute<glm::vec3> normals = LoadAttribute<glm::vec3>(model, mesh.primitives, "NORMAL");
			const LoadedAttribute<glm::vec2> uvs = LoadAttribute<glm::vec2>(model, mesh.primitives, "TEXCOORD_0");
			const LoadedAttribute<glm::vec4> tangents = LoadAttribute<glm::vec4>(model, mesh.primitives, "TANGENT");
			const LoadedIndices loadedIndices = LoadIndices(model, mesh.primitives, positions.Offsets);

			outMesh->Indices = loadedIndices.Indices;
			outMesh->Positions = positions.Data;
			outMesh->Normals = normals.Data;
			outMesh->UVs = uvs.Data;
			outMesh->Tangents = tangents.Data;

			// Generate tangents if they are not present in the GLTF file.
			if (outMesh->Tangents.empty())
			{
				MikkTSpaceTangent::MikktSpaceMesh m;
				m.m_indices = &outMesh->Indices;
				m.m_positions = &outMesh->Positions;
				m.m_normals = &outMesh->Normals;
				m.m_texcoords = &outMesh->UVs;

				MikkTSpaceTangent::GetTangents(m, outMesh->Tangents);
			}

			outMesh->Bitangents = CalculateBITangents(outMesh->Tangents, outMesh->Normals);

			// Create submeshes.
			for (size_t i = 0; i < mesh.primitives.size(); i++)
			{
				const auto& prim = mesh.primitives[i];
				const auto& accessor = model.accessors[prim.indices];

				MeshAsset::SubMesh subMesh;
				subMesh.IndexOffset = loadedIndices.Offsets[i];
				subMesh.IndexCount = static_cast<uint32_t>(accessor.count);

				if (prim.material >= 0)
				{
					subMesh.Material = materials[prim.material];
				}

				outMesh->SubMeshes.push_back(subMesh);
			}

			outMesh->GPULoad();

			std::string name = mesh.name;
			if (mesh.name.empty())
			{
				name = "Mesh_" + std::to_string(i);
			}
			const auto subMetaData = std::find_if(meta.Assets.begin(), meta.Assets.end(), [&](const auto& subMeta)
				{
					return subMeta.second.Name == name && subMeta.second.Type.TypeName == "Mesh";
				});

			if (subMetaData == meta.Assets.end())
			{
				BF_CORE_LOG_ERROR("Failed to find sub asset metadata for mesh: %s", name.c_str());
				return {};
			}

			meshes.push_back(manager.AddAssetEntry<MeshAsset>(AssetEntry{ subMetaData->second.AssetID, subMetaData->second.Type, outMesh }));
		}

		return meshes;
	}

	void LoadMeshMetadata(const tinygltf::Model& model, std::unordered_map<UUID, AssetMetadata>& metadata)
	{
		for (uint32_t i = 0; i < model.meshes.size(); i++)
		{
			AssetMetadata matMetadata;
			matMetadata.Name = model.meshes[i].name;
			if (model.meshes[i].name.empty())
			{
				matMetadata.Name = "Mesh_" + std::to_string(i);
			}
			matMetadata.Type = { "Mesh" };
			matMetadata.AssetID = UUID::Generate();

			metadata.insert({ matMetadata.AssetID, matMetadata });
		}
	}

	RefPtr<ModelNode> LoadNode(const tinygltf::Model& gltfModel, const tinygltf::Node& gltfNode, const std::vector<AssetHandle<MeshAsset>>& meshes)
	{
		RefPtr<ModelNode> newNode = MakeRef<ModelNode>();
		newNode->Name = gltfNode.name;
		if (gltfNode.matrix.size() == 16)
		{
			glm::mat4 matrix(1.0f);
			for (int i = 0; i < 16; ++i)
			{
				matrix[i / 4][i % 4] = static_cast<float>(gltfNode.matrix[i]);
			}
			newNode->ModelMatrix = matrix;
		}
		else
		{
			auto translation = glm::vec3(0.0f);
			auto rotation = glm::identity<glm::quat>();
			auto scale = glm::vec3(1.0f);
			if (gltfNode.translation.size() == 3)
			{
				translation = glm::vec3(
					gltfNode.translation[0],
					gltfNode.translation[1],
					gltfNode.translation[2]
				);
			}
			if (gltfNode.rotation.size() == 4)
			{
				rotation = glm::quat(
					gltfNode.rotation[3],
					gltfNode.rotation[0],
					gltfNode.rotation[1],
					gltfNode.rotation[2]
				);
			}
			if (gltfNode.scale.size() == 3)
			{
				scale = glm::vec3(
					gltfNode.scale[0],
					gltfNode.scale[1],
					gltfNode.scale[2]
				);
			}
			newNode->ModelMatrix = glm::translate(glm::mat4(1.0f), translation) * glm::toMat4(rotation) * glm::scale(glm::mat4(1.0f), scale);
		}
		if (gltfNode.mesh >= 0) newNode->Mesh = meshes[gltfNode.mesh];
		return newNode;
	}

	std::vector<RefPtr<ModelNode>> TraverseNode(const tinygltf::Model& gltfModel, const std::vector<int>& childrenIndices, const std::vector<AssetHandle<MeshAsset>>& meshes)
	{
		std::vector<RefPtr<ModelNode>> outNodes;
		outNodes.resize(childrenIndices.size());

		int childIndex = 0;
		for (int nodeIndex : childrenIndices)
		{
			RefPtr<ModelNode> newNode = LoadNode(gltfModel, gltfModel.nodes[nodeIndex], meshes);
			newNode->Children = TraverseNode(gltfModel, gltfModel.nodes[nodeIndex].children, meshes);

			outNodes[childIndex] = newNode;
			childIndex++;
		}

		return outNodes;
	}

	RefPtr<ModelNode> CreateNodes(const tinygltf::Model& gltfModel, const std::vector<AssetHandle<MeshAsset>>& meshes)
	{
		RefPtr<ModelNode> root = MakeRef<ModelNode>();

		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene];
		BF_CORE_ASSERT(scene.nodes.size() == 1, "Only single-root scenes are supported");

		const int rootNodeIndex = scene.nodes[0];
		const tinygltf::Node& gltfRoot = gltfModel.nodes[rootNodeIndex];
		root = LoadNode(gltfModel, gltfRoot, meshes);

		root->Children = TraverseNode(gltfModel, gltfRoot.children, meshes);

		return root;
	}

	bool GLTFImporter::CanImport(const std::string& fileExtention) const
	{
		std::string extension = fileExtention;
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		return extension == ".gltf" ||
			extension == ".glb";
	}

	bool GLTFImporter::CanImportType(const AssetType& type) const
	{
		return type == ModelAsset::Type;
	}

	bool GLTFImporter::Import(const AssetFileMetadata& meta, AssetManager& manager) const
	{
		tinygltf::Model gltfModel;
		LoadGltfModel(meta.Path.string(), gltfModel);

		std::vector<AssetHandle<MaterialAsset>> materials = LoadMaterials(gltfModel, manager, meta);
		std::vector<AssetHandle<MeshAsset>> meshes = LoadMeshes(gltfModel, materials, manager, meta);
		RefPtr<ModelNode> root = CreateNodes(gltfModel, meshes);

		RefPtr<ModelAsset> outModel = MakeRef<ModelAsset>();
		outModel->RootNode = root;

		manager.AddAssetEntry<ModelAsset>(AssetEntry{ meta.RootAssetID, ModelAsset::Type, outModel, true});

		return true;
	}

	bool GLTFImporter::CreateMeta(const std::filesystem::path& file, AssetFileMetadata& outMetadata) const
	{
		tinygltf::Model gltfModel;
		LoadGltfModel(file.string(), gltfModel);

		outMetadata.Path = file;
		outMetadata.SourceFileID = UUID::Generate();

		outMetadata.Assets.clear();
		LoadMeshMetadata(gltfModel, outMetadata.Assets);
		LoadMaterialMetadata(gltfModel, outMetadata.Assets);

		const auto uuid = UUID::Generate();

		std::string name = gltfModel.nodes[gltfModel.defaultScene].name;
		if (name.empty())
		{
			name = "Model_0";
		}
		outMetadata.Assets[uuid] = { outMetadata.SourceFileID, name, ModelAsset::Type, uuid };
		outMetadata.RootAssetID = uuid;
		outMetadata.SyncSourceFileIDWithAssets();
		return true;
	}
}