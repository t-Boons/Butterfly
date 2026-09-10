#pragma once
#include "Asset/AssetManager.hpp"

namespace Butterfly
{
	template<typename T>
	struct AssetHandle
	{
	public:
		bool Valid() const { return m_ID.Valid(); }
		const UUID& GetID() const { return m_ID; }

		AssetHandle() = default; // Used to create AssetHandles that are empty.

		// Copy and Move constructors used for reference counting.
		AssetHandle(const AssetHandle<T>& other);
		AssetHandle(AssetHandle<T>&& other) noexcept;
		AssetHandle<T>& operator=(const AssetHandle<T>& other);
		AssetHandle<T>& operator=(AssetHandle<T>&& other) noexcept;

		~AssetHandle();

	private:
		friend class AssetManager;

		// Used to create AssetHandles that are valid and point to an asset in the AssetManager.
		// This should and can only be called by the AssetManager.
		AssetHandle(AssetManager* manager, const UUID& id);

		AssetManager* m_manager = nullptr;
		UUID m_ID;
	};

	template<typename T>
	AssetHandle<T>::AssetHandle(const AssetHandle<T>& other) : m_manager(other.m_manager), m_ID(other.m_ID)
	{
		if (m_ID.Valid())
		{
			m_manager->AddRef(m_ID);
		}
	}

	template<typename T>
	AssetHandle<T>::AssetHandle(AssetHandle<T>&& other) noexcept : m_manager(other.m_manager), m_ID(other.m_ID)
	{
		other.m_manager = nullptr;
		other.m_ID = {};
	}

	template<typename T>
	AssetHandle<T>& AssetHandle<T>::operator=(const AssetHandle<T>& other)
	{
		if (this != &other)
		{
			if (m_ID.Valid())
			{
				m_manager->SubtractRef(m_ID);
			}
			m_manager = other.m_manager;
			m_ID = other.m_ID;
			if (m_ID.Valid())
			{
				m_manager->AddRef(m_ID);
			}
		}
		return *this;
	}

	template<typename T>
	AssetHandle<T>& AssetHandle<T>::operator=(AssetHandle<T>&& other) noexcept
	{
		if (this != &other)
		{
			if (m_ID.Valid())
			{
				m_manager->SubtractRef(m_ID);
			}
			m_manager = other.m_manager;
			m_ID = other.m_ID;
			other.m_manager = nullptr;
			other.m_ID = {};
		}
		return *this;
	}

	template<typename T>
	AssetHandle<T>::~AssetHandle()
	{
		if (m_ID.Valid())
		{
			m_manager->SubtractRef(m_ID);
		}
	}

	template<typename T>
	AssetHandle<T>::AssetHandle(AssetManager* manager, const UUID& id) : m_manager(manager), m_ID(id)
	{
		m_manager->AddRef(m_ID);
	}
}