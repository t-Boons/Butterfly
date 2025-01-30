#pragma once
#include "Core/Common.hpp"
#include "GraphicsAPI.hpp"
#include "Types.hpp"

namespace Butterfly
{
	class CommandList : private NonCopyable
	{
	public:
		virtual ~CommandList() = default;
		virtual void Reset() = 0;
		virtual void Close() = 0;
		virtual void Marker(const std::string& str) = 0;
		virtual void BeginGPUMarker(const std::string& str) = 0;
		virtual void EndGPUMarker() = 0;

		QueueType Type() const { return m_type; }

	private:


		QueueType m_type;
	};
}