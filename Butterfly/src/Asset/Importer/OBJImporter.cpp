#include "Asset/Importer/OBJImporter.hpp"
#include "Asset/AssetTypes.hpp"
#include "tinyobj/tiny_obj_loader.h"

namespace Butterfly
{
	BF_REGISTER_ASSET_IMPORTER(OBJImporter);

	bool OBJImporter::CanImport(const std::string& fileExtention) const
	{
		std::string extension = fileExtention;
		std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
		return extension == ".obj";
	}

	bool OBJImporter::CanImportType(const AssetType& type) const
	{
		return type == MeshAsset::Type() ||
			   type == ModelAsset::Type();
	}

	bool OBJImporter::Import(const AssetFileMetadata& meta, AssetManager& manager) const
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string err;

		bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, meta.Path.string().c_str(), meta.Path.parent_path().string().c_str(), true);

		if (!ok)
		{
			BF_CORE_LOG_ERROR("OBJImporter failed to load file, reason:");
			BF_CORE_LOG_ERROR("%s", err);
			return false;
		}

		const uint32_t vertexElementCount = 3;

		if (shapes.size() < 1)
		{
			BF_CORE_LOG_ERROR("OBJImporter failed. Mesh is empty.");
			return false;
		}

		RefPtr<MeshAsset> mesh = MakeRef<MeshAsset>();

		mesh->Indices.resize(shapes[0].mesh.indices.size());
		std::iota(mesh->Indices.begin(), mesh->Indices.end(), 0);

		uint32_t indexOffset = 0;
		for (uint32_t f = 0; f < shapes[0].mesh.num_face_vertices.size(); f++)
		{
			for (uint32_t v = 0; v < vertexElementCount; v++)
			{
				tinyobj::index_t idx = shapes[0].mesh.indices[indexOffset + v];

				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

				tinyobj::real_t uvx = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t uvy = attrib.texcoords[2 * idx.texcoord_index + 1];


				mesh->Positions.push_back({ vx, vy, vz });
				mesh->Normals.push_back({ nx, ny, nz });
				mesh->UVs.push_back({ uvx, uvy });
			}
			indexOffset += vertexElementCount;
		
		}

		mesh->SubMeshes.push_back({ 0, static_cast<uint32_t>(mesh->Indices.size()) });

		mesh->GPULoad();


		manager.AddAssetEntry<MeshAsset>(AssetEntry{ meta.RootAssetID, MeshAsset::Type(), StaticCastRef<void>(mesh) });
		return true;
	}

	bool OBJImporter::CreateMeta(const std::filesystem::path& file, AssetFileMetadata& meta) const
	{
		meta.SourceFileID = UUID::Generate();
		meta.Path = file;

		const auto uuid = UUID::Generate();
		meta.RootAssetID = uuid;
		meta.Assets[uuid] = AssetMetadata{ meta.SourceFileID, file.filename().string(), MeshAsset::Type(), uuid };
		meta.SyncSourceFileIDWithAssets();
		return true;
	}
}