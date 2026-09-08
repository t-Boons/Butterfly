#pragma once

namespace Butterfly
{
	class D3D12CommandList;

	class Application;
	class Layer : public NonCopyable
	{
	public:
		virtual void OnInit() {}
		virtual void OnTick() {}
		virtual void OnShutdown() {}
		virtual void OnRender(D3D12CommandList& list) {}
	};
}