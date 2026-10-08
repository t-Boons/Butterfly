#pragma once
#include "Core/Common.hpp"
#include "Renderer/Renderer.hpp"

namespace Butterfly
{
	class ViewportRenderEvent;

	class IRenderPipelineStage
	{
	public:
		virtual void OnPreRender(const ViewportPrerenderEvent& ev) {}
		virtual void OnRecordPass(const ViewportRenderEvent& ev) = 0;
		virtual void OnPostRender(const ViewportPostRenderEvent& ev) {}
	};

	class RenderPipeline
	{
	public:
		RenderPipeline(Renderer& renderer);

		template<typename T>
		T& RegisterStage()
		{
			if (T* existingStage = TryGetStage<T>())
			{
				BF_CORE_LOG_WARN("RenderPipeline stage already exists: %s", typeid(T).name());
				return *existingStage;
			}
			BF_CORE_LOG_TRACE("Attaching renderpipelinestage: %s", typeid(T).name());
			m_renderPipelineStages.push_back(MakeRef<T>());
			return *DynamicCastRef<T>(m_renderPipelineStages[m_renderPipelineStages.size() - 1]).get();
		}

		template<typename T>
		T* TryGetStage()
		{
			for (const auto& stage : m_renderPipelineStages)
			{
				if (RefPtr<T> castedStage = DynamicCastRef<T>(stage))
				{
					return castedStage.get();
				}
			}
			return nullptr;
		}

		void PreRender(const ViewportPrerenderEvent& ev);
		void RecordPasses(const ViewportRenderEvent& ev);
		void PostRender(const ViewportPostRenderEvent& ev);

		void ClearStages();
		void SetDefaultStages();

	private:
		std::vector<RefPtr<IRenderPipelineStage>> m_renderPipelineStages;
		Renderer& m_renderer;
	};
}