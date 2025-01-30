#pragma once
#include "Core/Common.hpp"
#include "Types.hpp"

namespace Butterfly
{
	enum class GraphicsAPIType
	{
		D3D12 = 0,
		Num
	};


	class CommandList;

	class GraphicsAPI
	{
	public:
		static void Init(GraphicsAPIType api, bool enableDebug);
		virtual ~GraphicsAPI() = default;

		virtual RefPtr<CommandList> CreateCommandList(QueueType type) const = 0;



		GraphicsAPIType API;
	protected:
		virtual void _Init(bool enableDebug) = 0;
	};

	extern GraphicsAPI* Graphics;
}