#pragma once

namespace Butterfly
{
	class Application;
	class Layer : public NonCopyable
	{
	public:
		virtual void OnInit() {}
		virtual void OnTick() {}
		virtual void OnShutdown() {}
	};
}