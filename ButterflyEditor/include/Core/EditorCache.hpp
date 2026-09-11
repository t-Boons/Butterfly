#pragma once
#include "Butterfly.hpp"
#include <yaml-cpp/yaml.h>

namespace Butterfly
{
	class IEditorCacheEntry
	{
	public:
		virtual std::vector<uint8_t> Serialize() const = 0;
		virtual void Deserialize(const std::vector<uint8_t>& data) = 0;
	};

	struct CachedFileMetadata
	{
		UUID ID;
		std::string CachedDataType;
		std::vector<uint8_t> Data;
	};
	
	class EditorCache
	{
	public:
		EditorCache()
		{
			m_editorCachePath = FileSystem::WorkingDirectory() / "EditorCache";
			Scan();
		}

		template<typename T>
		bool Get(UUID id, T& out)
		{
			auto it = m_metaEntries.find(id);
			if (it == m_metaEntries.end())
			{
				return false;
			}

			auto entry = m_loadedEntries.find(id);
			if (entry == m_loadedEntries.end())
			{
				RefPtr<T> entryData = MakeRef<T>();
				entryData->Deserialize(it->second.Data);
				m_loadedEntries[id] = entryData;
			}

			out = *StaticCastRef<T>(m_loadedEntries[id]);

			return true;
		}

		bool Exists(UUID id)
		{
			return m_metaEntries.find(id) != m_metaEntries.end();
		}


		void Scan()
		{
			const std::vector<std::filesystem::path> files = FileSystem::WalkDirectoryRecursive(m_editorCachePath);

			for (auto& file : files)
			{
				if (file.extension() == m_metaFileExtention)
				{
					YAML::Node node;
					node = YAML::LoadFile(file.string());

					CachedFileMetadata meta = node["Meta"].as<CachedFileMetadata>();
					m_metaEntries[meta.ID] = meta;
				}
			}
		}


		template<typename T>
		void Add(UUID id, const T& data)
		{
			if(!Exists(id))
			{
				CachedFileMetadata meta;
				meta.ID = id;
				meta.CachedDataType = typeid(T).name();
				meta.Data = data.Serialize();

				YAML::Node node;
				node["Meta"] = meta;
				YAML::Emitter emitter;
				emitter << node;

				if(!FileSystem::WriteText(m_editorCachePath / (id.ToString() + m_metaFileExtention), emitter.c_str()))
				{
					BF_CORE_LOG_ERROR("Failed to write CachedFileMetadata");
				}

				m_metaEntries[id] = meta;
			}
		}

	private:
		std::unordered_map<UUID, CachedFileMetadata> m_metaEntries;
		 std::unordered_map<UUID, RefPtr<void>> m_loadedEntries;
		 std::filesystem::path m_editorCachePath;
		 const std::string m_metaFileExtention = ".bfcache";
	};
}

namespace YAML
{
	template<>
	struct convert<Butterfly::CachedFileMetadata>
	{
		static Node encode(const Butterfly::CachedFileMetadata& metadata)
		{
			Node node;

			node["CachedDataType"] = metadata.CachedDataType;
			node["ID"] = metadata.ID.ToString();
			node["Data"] = YAML::Binary(metadata.Data.data(), metadata.Data.size());

			return node;
		}

		static bool decode(const Node& node, Butterfly::CachedFileMetadata& metadata)
		{
			if (!node.IsMap())
			{
				return false;
			}

			metadata.CachedDataType = node["CachedDataType"].as<std::string>();
			metadata.ID = Butterfly::UUID::FromString(node["ID"].as<std::string>());

			const auto binary = node["Data"].as<YAML::Binary>();
			metadata.Data.assign(binary.data(), binary.data() + binary.size());

			return true;
		}
	};
}