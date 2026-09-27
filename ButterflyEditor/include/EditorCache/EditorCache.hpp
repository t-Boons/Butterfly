#pragma once
#include "Butterfly.hpp"
#include "EditorCache/EditorCacheFile.hpp"

namespace Butterfly
{
	template<typename T>
	class EditorCacheHandle
	{
	public:
		bool Loaded = false;
		RefPtr<T> Data;
	};

	class IEditorCacheEntry
	{
	public:
		virtual std::vector<uint8_t> Serialize() const = 0;
		virtual void Deserialize(const std::vector<uint8_t>& data) = 0;
	};

	class EditorCache
	{
	public:
		EditorCache();

		template<typename T>
		bool Get(UUID id, EditorCacheHandle<T>& out);

		template<typename T>
		void Add(UUID id, const T& data);

		bool Exists(UUID id);
		void Scan();

	private:
		std::mutex m_loadedEntriesMutex;
		std::unordered_map<UUID, EditorCacheFileMetadata> m_metaEntries;
		std::unordered_map<UUID, RefPtr<void>> m_loadedEntries;
		std::filesystem::path m_editorCachePath;
		const std::string m_metaFileExtention = ".bfcache";
	};


	inline EditorCache::EditorCache()
	{
		m_editorCachePath = FileSystem::WorkingDirectory() / "EditorCache";
		Scan();
	}

	template<typename T>
	inline bool EditorCache::Get(UUID id, EditorCacheHandle<T>& out)
	{
		auto it = m_metaEntries.find(id);
		if (it == m_metaEntries.end())
		{
			return false;
		}

		auto entry = m_loadedEntries.find(id);
		if (entry == m_loadedEntries.end())
		{
			Application::Get().GetJobSystem().Submit([this, id]()
				{
					std::lock_guard<std::mutex> lock(m_loadedEntriesMutex);

					BF_CORE_LOG_TRACE("Loading EditorCache entry with ID: %s", id.ToString().c_str());
					auto it = m_metaEntries.find(id);
					if (it == m_metaEntries.end())
					{
						BF_CORE_LOG_ERROR("EditorCache entry with ID: %s not found in meta entries", id.ToString().c_str());
						return;
					}
					RefPtr<IEditorCacheEntry> entryData = MakeRef<T>();
					entryData->Deserialize(it->second.Data);
					m_loadedEntries[id] = entryData;
				});

			out.Loaded = false;
			return true;
		}

		std::lock_guard<std::mutex> lock(m_loadedEntriesMutex);
		out.Loaded = true;
		out.Data = StaticCastRef<T>(m_loadedEntries[id]);

		return true;
	}

	inline bool EditorCache::Exists(UUID id)
	{
		std::lock_guard<std::mutex> lock(m_loadedEntriesMutex);
		return m_metaEntries.find(id) != m_metaEntries.end();
	}


	inline void EditorCache::Scan()
	{
		const std::vector<std::filesystem::path> files = FileSystem::WalkDirectoryRecursive(m_editorCachePath);
		BF_CORE_LOG_INFO("EditorCache::Scan: NumFiles %d, Directory: %s", files.size(), m_editorCachePath.string().c_str());

		for (auto& file : files)
		{
			if (file.extension() == m_metaFileExtention)
			{
				YAML::Node node;
				node = YAML::LoadFile(file.string());

				EditorCacheFileMetadata meta = node["Meta"].as<EditorCacheFileMetadata>();
				std::lock_guard<std::mutex> lock(m_loadedEntriesMutex);
				m_metaEntries[meta.ID] = meta;
			}
		}
	}

	template<typename T>
	inline void EditorCache::Add(UUID id, const T& data)
	{
		if (!Exists(id))
		{
			BF_CORE_LOG_TRACE("Adding EditorCache entry with ID: %s", id.ToString().c_str());
			EditorCacheFileMetadata meta;
			meta.ID = id;
			meta.CachedDataType = typeid(T).name();
			meta.Data = data.Serialize();

			YAML::Node node;
			node["Meta"] = meta;
			YAML::Emitter emitter;
			emitter << node;

			if (!FileSystem::WriteText(m_editorCachePath / (id.ToString() + m_metaFileExtention), emitter.c_str()))
			{
				BF_CORE_LOG_ERROR("Failed to write CachedFileMetadata");
			}

			m_metaEntries[id] = meta;
		}
	}
}