#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Butterfly
{
	namespace ImGUIHelpers
	{
		static bool DrawFloatControl(const char* label, glm::vec2& values, float resetValue = 0.0f)
		{
			bool changed = false;
			ImGuiIO& io = ImGui::GetIO();
			auto boldFont = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

			ImGui::PushID(label);

			float fullWidth = ImGui::GetContentRegionAvail().x;
			float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight, lineHeight };

			float groupSpacing = ImGui::GetStyle().ItemSpacing.x;
			float dragWidth = (fullWidth - 3.0f * buttonSize.x - 2.0f * groupSpacing) / 3.0f;

			auto axisControl = [&](const char* axisLabel, float& v, bool isLast,
				ImVec4 baseColor) {
					ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
					ImGui::PushFont(boldFont);
					ImGui::Button(axisLabel, buttonSize);
					ImGui::PopFont();
					ImGui::PopStyleColor(2);

					ImGui::SameLine(0.0f, 0.0f);

					ImGui::PushItemWidth(dragWidth);
					std::string dragId = std::string("##") + axisLabel;
					char fmt[] = "%.3f";
					changed |= ImGui::InputScalar(dragId.c_str(), ImGuiDataType_Float, &v, nullptr, nullptr, fmt);
					ImGui::PopItemWidth();

					if (!isLast)
					{
						ImGui::SameLine(0.0f, groupSpacing);
					}
				};

			axisControl("X", values.x, true, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));

			ImGui::PopID();

			return changed;
		}

		static bool DrawVec2Control(const char* label, glm::vec2& values, float resetValue = 0.0f)
		{
			bool changed = false;
			ImGuiIO& io = ImGui::GetIO();
			auto boldFont = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

			ImGui::PushID(label);

			float fullWidth = ImGui::GetContentRegionAvail().x;
			float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight, lineHeight };

			float groupSpacing = ImGui::GetStyle().ItemSpacing.x;
			float dragWidth = (fullWidth - 3.0f * buttonSize.x - 2.0f * groupSpacing) / 3.0f;

			auto axisControl = [&](const char* axisLabel, float& v, bool isLast,
				ImVec4 baseColor) {
					ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
					ImGui::PushFont(boldFont);
					ImGui::Button(axisLabel, buttonSize);
					ImGui::PopFont();
					ImGui::PopStyleColor(2);

					ImGui::SameLine(0.0f, 0.0f);

					ImGui::PushItemWidth(dragWidth);
					std::string dragId = std::string("##") + axisLabel;
					char fmt[] = "%.3f";
					changed |= ImGui::InputScalar(dragId.c_str(), ImGuiDataType_Float, &v, nullptr, nullptr, fmt);
					ImGui::PopItemWidth();

					if (!isLast)
					{
						ImGui::SameLine(0.0f, groupSpacing);
					}
				};

			axisControl("X", values.x, false, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));
			axisControl("Y", values.y, true, ImVec4(0.16f, 0.55f, 0.16f, 1.0f));

			ImGui::PopID();

			return changed;
		}

		static bool DrawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f)
		{
			bool changed = false;
			ImGuiIO& io = ImGui::GetIO();
			auto boldFont = io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];

			ImGui::PushID(label);

			float fullWidth = ImGui::GetContentRegionAvail().x;
			float lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
			ImVec2 buttonSize = { lineHeight, lineHeight };

			float groupSpacing = ImGui::GetStyle().ItemSpacing.x;
			float dragWidth = (fullWidth - 3.0f * buttonSize.x - 2.0f * groupSpacing) / 3.0f;

			auto axisControl = [&](const char* axisLabel, float& v, bool isLast,
				ImVec4 baseColor) {
					ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
					ImGui::PushFont(boldFont);
					ImGui::Button(axisLabel, buttonSize);
					ImGui::PopFont();
					ImGui::PopStyleColor(2);

					ImGui::SameLine(0.0f, 0.0f);

					ImGui::PushItemWidth(dragWidth);
					std::string dragId = std::string("##") + axisLabel;
					char fmt[] = "%.3f";
					changed |= ImGui::InputScalar(dragId.c_str(), ImGuiDataType_Float, &v, nullptr, nullptr, fmt);
					ImGui::PopItemWidth();

					if (!isLast)
					{
						ImGui::SameLine(0.0f, groupSpacing);
					}
				};

			axisControl("X", values.x, false, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));
			axisControl("Y", values.y, false, ImVec4(0.16f, 0.55f, 0.16f, 1.0f));
			axisControl("Z", values.z, true, ImVec4(0.14f, 0.28f, 0.72f, 1.0f));

			ImGui::PopID();

			return changed;
		}
	}
}