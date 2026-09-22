#pragma once
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "imgui/imgui_internal.h"
#include "ImGUI/FontAwesomeIcons.hpp"

namespace Butterfly
{
	namespace ImGUIHelpers
	{
		static bool DrawFloatControl(const char* label, float& value, float resetValue = 0.0f)
		{
			bool changed = false;

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
					ImGui::Button(axisLabel, buttonSize);
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

			axisControl("X", value, true, ImVec4(0.72f, 0.16f, 0.16f, 1.0f));

			ImGui::PopID();

			return changed;
		}

		static bool DrawVec2Control(const char* label, glm::vec2& values, float resetValue = 0.0f)
		{
			bool changed = false;

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
					ImGui::Button(axisLabel, buttonSize);
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
					ImGui::Button(axisLabel, buttonSize);
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

		static void TextWrappedCentered(const std::string& text, const ImVec2& start, float maxWidth)
		{
			ImFont* font = ImGui::GetFont();
			const float fontSize = ImGui::GetFontSize();
			const float lineHeight = ImGui::GetTextLineHeight();

			std::vector<std::string> lines;
			std::string currentLine;

			for (const char c : text)
			{
				std::string testLine = currentLine + c;

				if (font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, testLine.c_str()).x > maxWidth && !currentLine.empty())
				{
					lines.push_back(currentLine);
					currentLine = c;
				}
				else
				{
					currentLine = testLine;
				}
			}

			if (!currentLine.empty())
			{
				lines.push_back(currentLine);
			}

			for (uint32_t i = 0; i < lines.size(); ++i)
			{
				const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, lines[i].c_str());

				const float x = start.x + (maxWidth - textSize.x) * 0.5f;
				const float y = start.y + i * lineHeight;

				ImGui::GetWindowDrawList()->AddText(font, fontSize, ImVec2(x, y), IM_COL32(255, 255, 255, 255), lines[i].c_str());
			}
		}

		static bool LabelledCheckmark(const char* label, bool* v)
		{
			ImGui::PushID(label);
			bool changed = ImGui::Checkbox("", v);
			ImGui::SameLine();
			ImGui::TextUnformatted(label);
			ImGui::PopID();
			return changed;
		}


		static bool AssetReferenceField(const UUID& uuid, const std::string& labelname, const std::string& assetReferenceTypeName, const std::string& name, bool referenceIsSet, std::function<void(UUID)> onDropNewReference)
		{
			const ImVec4 buttonRefColor = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
			const ImVec4 buttonNoRefColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
			const ImVec4 buttonColor = uuid ? buttonRefColor : buttonNoRefColor;
			ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));

			const float fieldHeight = 20.0f;
			const float fieldWidth = 200.0f;
			const float pickerWidth = fieldHeight;

			ImGui::Text((std::string(labelname) + ": ").c_str());
			ImGui::SameLine(0.0f, 0.0f);
			ImGui::Button(FontAwesome::Search, ImVec2(pickerWidth, fieldHeight));
			ImGui::SameLine(0.0f, 0.0f);

			std::stringstream buttonText;
			if(referenceIsSet)
			{
				buttonText << name;
			}
			else
			{
				buttonText << "No Reference " << "(" << assetReferenceTypeName << ")";
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
			ImGui::Button(buttonText.str().c_str(), ImVec2(fieldWidth, fieldHeight));
			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					if (payload->DataSize == sizeof(UUID))
					{
						const UUID payloadUUID = *static_cast<const UUID*>(payload->Data);

						if (uuid != payloadUUID)
						{
							onDropNewReference(payloadUUID);
						}

						ImGui::EndDragDropTarget();
						ImGui::PopStyleColor(3);
						return true;
					}
				}

				ImGui::EndDragDropTarget();
			}

			ImGui::PopStyleColor(3);
			return false;
		}
	}
}