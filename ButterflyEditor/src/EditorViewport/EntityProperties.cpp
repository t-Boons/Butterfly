#include "EditorViewport/EntityProperties.hpp"
#include "EditorViewport/EditorViewport.hpp"
#include "Core/EditorApplication.hpp"
#include "ImGui/ImGuiHelpers.hpp"
#include "ImGui/FontAwesomeIcons.hpp"

namespace Butterfly
{

    bool InspectorValue(const entt::meta_data& data, auto& component, entt::meta_any& value, const char* name)
    {
        const entt::meta_type& type = value.type();

        if (type == entt::resolve<std::string>())
        {
            std::string s = value.cast<std::string>();

            ImGui::TextUnformatted((std::string(name) + ": ").c_str());
            ImGui::SameLine();

            if (ImGui::InputText("##input", &s))
            {
                value = s;
                data.set(component, value);
                return true;
            }
            return false;
        }

        if (type == entt::resolve<glm::vec2>())
        {
            glm::vec2 v = value.cast<glm::vec2>();

            if (ImGUIHelpers::DrawVec2Control(name, v))
            {
                value = v;
                data.set(component, value);
                return true;
            }

            return false;
        }

        if (type == entt::resolve<glm::vec3>())
        {
            glm::vec3 v = value.cast<glm::vec3>();

            if (ImGUIHelpers::DrawVec3Control(name, v))
            {
                value = v;
                data.set(component, value);
                return true;
            }

            return false;
        }

        if (type == entt::resolve<glm::quat>())
        {
            glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(value.cast<glm::quat>()));

            if (ImGUIHelpers::DrawVec3Control(name, eulerRotation))
            {
                value = glm::quat(glm::radians(eulerRotation));
                data.set(component, value);
                return true;
            }

            return false;
        }

        if (type == entt::resolve<float>())
        {
            float v = value.cast<float>();

            if (ImGUIHelpers::DrawFloatControl(name, v))
            {
                value = v;
                data.set(component, value);
                return true;
            }

            return false;
        }

        if (type == entt::resolve<bool>())
        {
            bool v = value.cast<bool>();

            if (ImGUIHelpers::LabelledCheckmark(name, &v))
            {
                value = v;
                data.set(component, value);
                return true;
            }   

            return false;
        }

		if (type == entt::resolve<AssetUUID<MeshAsset>>())
		{
			std::string assetName = "";
			const UUID& uuid = value.cast<AssetUUID<MeshAsset>>().ID();
            if (uuid)
            {
                AssetMetadata meta;
                Application::Get().GetAssetManager().GetAssetRegistry().FindAsset(uuid, meta);
                assetName = meta.Name;
            }

			ImGUIHelpers::AssetReferenceField(uuid, name, "Mesh", assetName.c_str(), uuid.Valid(), [&](UUID newReference)
                {
                    AssetMetadata meta;
                    Application::Get().GetAssetManager().GetAssetRegistry().FindAsset(newReference, meta);

                    if (meta.Type == MeshAsset::Type)
                    {
                        value = AssetUUID<MeshAsset>{ newReference };
                        data.set(component, value);
                    }
                });
		}

        if (type == entt::resolve<AssetUUID<TextureAsset>>())
        {
            std::string assetName = "";
            const UUID& uuid = value.cast<AssetUUID<TextureAsset>>().ID();
            if (uuid)
            {
                AssetMetadata meta;
                Application::Get().GetAssetManager().GetAssetRegistry().FindAsset(uuid, meta);
                assetName = meta.Name;
            }

            ImGUIHelpers::AssetReferenceField(uuid, name, "Texture", assetName.c_str(), uuid.Valid(), [&](UUID newReference)
                {
                    AssetMetadata meta;
                    Application::Get().GetAssetManager().GetAssetRegistry().FindAsset(newReference, meta);
                    if (meta.Type == TextureAsset::Type)
                    {
                        value = AssetUUID<TextureAsset>{ newReference };
                        data.set(component, value);
                    }
                });
        }

		if (type == entt::resolve<EntityUUID>())
		{
            std::string assetName = "";
            const UUID& uuid = value.cast<AssetUUID<MeshAsset>>().ID();

            ImGUIHelpers::AssetReferenceField(uuid, name, "Entity", assetName.c_str(), uuid.Valid(), [&](UUID newReference)
                {

                });
		}


        return false;
    }

	EntityProperties::EntityProperties()
	{
	}

	EntityProperties::~EntityProperties()
	{
	}

	void EntityProperties::OnTick()
	{
	}

	void EntityProperties::OnRenderImGUI()
	{
		ImGui::Begin("Properties");

		Entity& selectedEntity = EditorApplication::Get().GetEditorViewport().m_selectedEntity;

		if (!selectedEntity)
		{
			ImGui::End();
			return;
		}

		ComponentRegistry::RunOnAllComponents([&]<typename T>()
		{
			if (!selectedEntity.HasComponent<T>())
			{
				return;
			}

			T& component = selectedEntity.GetComponent<T>();
			entt::meta_type type = entt::resolve<T>();

			if (!Utils::HasFlag(type.traits<ComponentMetaFlags>(), ComponentMetaFlags::Inspector))
			{
				return;
			}

			const std::string_view componentName = type.name();

			if (ImGui::CollapsingHeader(componentName.data(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::PushID(componentName.data());

				for (const auto& [id, data] : type.data())
				{
                    if (!Utils::HasFlag(data.traits<ComponentMetaFlags>(), ComponentMetaFlags::Inspector))
                    {
                        continue;
                    }

					const char* dataName = data.name().data();

					ImGui::PushID(dataName);
					entt::meta_any value = data.get(component);

					InspectorValue(data, component, value, dataName);

					ImGui::PopID();
				}

				ImGui::PopID();
			}
		});

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		const ImVec2 size = ImGui::GetContentRegionAvail();
		if (ImGui::Button("Add Component", ImVec2(size.x, 20.0f)))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup"))
		{
			ComponentRegistry::RunOnAllComponents([&]<typename T>()
			{
				if (!selectedEntity.HasComponent<T>())
				{
					const std::string_view componentName = entt::resolve<T>().name();
					if (ImGui::MenuItem(componentName.data()))
					{
						selectedEntity.AddComponent<T>();
						ImGui::CloseCurrentPopup();
					}
				}
			});

			ImGui::EndPopup();
		}

		ImGui::End();
	}
}