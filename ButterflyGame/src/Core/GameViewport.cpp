#include "Core/GameViewport.hpp"

#include "Core/GameApplication.hpp"
#include "Core/GameViewport.hpp"

namespace Butterfly::Game
{
	GameViewport::GameViewport()
	{
		m_viewportHandle = Application::Get().GetRenderer().AddViewport();
		m_imGUIRenderReceiver.Subscribe(Application::Get().GetRenderer().GetImGUIRenderEvent(), BF_BIND_FUNC(&GameViewport::OnRenderImGUI));
		m_viewportPrerenderReceiver.Subscribe(Application::Get().GetRenderer().GetViewport(m_viewportHandle).Events.OnPreRender, BF_BIND_FUNC_PARAM(&GameViewport::OnPrerender));
	}

	GameViewport::~GameViewport()
	{
		Application::Get().GetRenderer().RemoveViewport(m_viewportHandle);
	}

	void GameViewport::OnTick()
	{
	}

	void GameViewport::OnPrerender(const ViewportPrerenderEvent& event)
	{
		BF_PROFILE_EVENT()

		glm::mat4 viewProjection = glm::mat4(0.0f);
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		glm::vec3 cameraPosition = glm::vec3(0.0f);
		float zNear = 0.0f, zFar = 0.0f;
		bool cameraFound = false;
		for (const auto& [entity, camera, transform] : Application::Get().GetScene().GetEntityRegistry().view<CameraComponent, TransformComponent>().each())
		{
			if (camera.GetCameraIndex() == m_mainCameraIndex)
			{
				camera.SetAspectRatio(static_cast<float>(event.Viewport.Size().x) / static_cast<float>(event.Viewport.Size().y));

				cameraFound = true;
				cameraPosition = transform.GetPosition();
				view = camera.GetViewMatrix(transform.GetPosition(), transform.GetRotation());
				viewProjection = camera.GetViewprojectionMatrrix(transform.GetPosition(), transform.GetRotation());
				projection = camera.GetProjectionMatrix();
				zNear = camera.GetZNear();
				zFar = camera.GetZFar();
				break;
			}
		}

		if (cameraFound != m_oldCameraFound)
		{
			m_shouldUpdateRenderPipeline = true;
			m_oldCameraFound = cameraFound;
		}

		if (cameraFound && m_shouldUpdateRenderPipeline)
		{
			Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->SetDefaultStages();
			m_shouldUpdateRenderPipeline = false;
		}

		if (!cameraFound && m_shouldUpdateRenderPipeline)
		{
			Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->ClearStages();
			Application::Get().GetRenderer().GetViewport(m_viewportHandle).RenderPipeline->RegisterStage<ClearRenderPipelineStage>().SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
			m_shouldUpdateRenderPipeline = false;
		}

		CameraData cameraData;
		cameraData.ViewProjection = viewProjection;
		cameraData.CameraPosition = cameraPosition;
		cameraData.InverseView = glm::transpose(glm::inverse(view));
		cameraData.InverseProjection = glm::transpose(glm::inverse(projection));
		cameraData.Near = zNear;
		cameraData.Far = zFar;
		event.Viewport.Uniforms->GetOrCreateView(sizeof(CameraData), HASH("CameraData"));
		event.Viewport.Uniforms->Write(&cameraData, sizeof(CameraData), HASH("CameraData"));
		event.Viewport.UniformCameraData = cameraData;

		event.Viewport.Lights->Update();
		event.Viewport.Materials->Validate();
		event.Viewport.Models->Update();

		if (SkyboxRenderPipelineStage* srps = event.Viewport.RenderPipeline->TryGetStage<SkyboxRenderPipelineStage>())
		{
			auto view = Application::Get().GetScene().GetEntityRegistry().view<SkyboxComponent>();
			auto first = view.begin();
			if (first != view.end())
			{
				SkyboxComponent& sb = view.get<SkyboxComponent>(*first);
				srps->Update(&sb);
			}
			else
			{
				srps->Update(nullptr);
			}
		}
	}

	void GameViewport::OnRenderImGUI()
	{
		BF_PROFILE_EVENT()

		const std::string windowName = "Game Viewport##" + std::to_string(m_viewportHandle.Index());

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

		// Viewport rendering.
		if (m_viewportHandle.Valid())
		{
			Application::Get().GetRenderer().ImGUIImage(m_viewportHandle);
		}

		if (!m_oldCameraFound)
		{
			const ImVec2 textSize = ImGui::CalcTextSize("NO CAMERA");
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 windowPos = ImGui::GetWindowPos();
			ImGui::GetWindowDrawList()->AddText(ImVec2(windowPos.x + (windowSize.x - textSize.x) * 0.5f, windowPos.y + (windowSize.y - textSize.y) * 0.5f), IM_COL32(255, 225, 225, 255), "NO CAMERA");
		}
		ImGui::End();
		ImGui::PopStyleVar(3);
	}
}