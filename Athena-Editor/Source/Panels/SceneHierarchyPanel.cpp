#include "SceneHierarchyPanel.h"

#include "Athena/Core/PlatformUtils.h"

#include "Athena/Input/Input.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Scene/Components.h"

#include "Athena/Scripting/ScriptEngine.h"

#include "Athena/UI/Widgets.h"

#include <ImGui/imgui.h>


namespace Athena
{
	void DrawVec3Property(std::string_view label, Vector3& values, float defaultValues)
	{
		UI::Property(label);

		ImGui::PushID(label.data());

		float full_width = ImGui::GetContentRegionAvail().x - 15.f;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

		float buttonWidth = ImGui::GetFrameHeight();
		ImVec2 buttonSize = { buttonWidth, buttonWidth };

		float dragWidth = (full_width - 3 * buttonWidth) / 3.f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f });
		UI::PushBoldFont();
		if (ImGui::Button("X", buttonSize))
			values.x = defaultValues;
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##X", &values.x, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f });
		UI::PushBoldFont();
		if (ImGui::Button("Y", buttonSize))
			values.y = defaultValues;
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Y", &values.y, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f });
		UI::PushBoldFont();
		if (ImGui::Button("Z", buttonSize))
			values.z = defaultValues;
		ImGui::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Z", &values.z, 0.7f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();

		ImGui::PopID();
	}


	SceneHierarchyPanel::SceneHierarchyPanel(std::string_view name, const Ref<Scene>& context)
		: Panel(name)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = Entity{};
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		DrawEnvironmentEditor(m_Context->GetEnvironment());

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Materials Editor");
		if (m_SelectionContext)
		{
			DrawMaterialsEditor();
		}
		ImGui::PopStyleVar();
		ImGui::End();

		DrawEntitiesHierarchy();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Properties");
		if (m_SelectionContext)
		{
			DrawAllComponents(m_SelectionContext);
		}
		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntitiesHierarchy()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetDarkColor());
		ImGui::Begin("Scene Hierarchy");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
		ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
		ImGui::PushStyleColor(ImGuiCol_Header, color);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

		Entity root = m_Context->GetRootEntity();
		DrawEntityNode(root, true);

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_SelectionContext = {};

		if (ImGui::BeginPopupContextWindow(0, 1, false))
		{
			if (ImGui::MenuItem("Create Entity"))
			{
				Entity created = m_Context->CreateEntity();
				m_SelectionContext = created;
			}

			ImGui::EndPopup();
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity, bool open)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;
		bool selected = m_SelectionContext == entity;
		bool hasChildren = entity.HasChildren();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_FramePadding;

		if (open)
			flags |= ImGuiTreeNodeFlags_DefaultOpen;

		if (selected)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (!hasChildren)
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		if(selected)
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive]);

		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (selected)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("SCENE_HIERARCHY_ENTITY", &entity, sizeof(entity));
			ImGui::Text(tag.c_str());
			ImGui::EndDragDropSource();
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY"))
			{
				Entity payloadEntity = *(Entity*)payload->Data;
				if (payloadEntity != entity)
				{
					m_Context->MakeParent(entity, payloadEntity);
				}
			}

			ImGui::EndDragDropTarget();
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (m_SelectionContext && !selected && ImGui::MenuItem("Add to children"))
			{
				Entity parent = entity;
				Entity child = m_SelectionContext;
				m_Context->MakeParent(parent, child);
			}

			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened && hasChildren)
		{
			const std::vector<Entity>& children = entity.GetComponent<ParentComponent>().Children;
			for (auto entity : children)
			{
				DrawEntityNode(entity);
			}
		}

		if (opened && hasChildren)
			ImGui::TreePop();

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	void SceneHierarchyPanel::DrawEnvironmentEditor(const Ref<Environment>& environment)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Environment");

		if (UI::TreeNode("Environment") && UI::BeginPropertyTable())
		{
			UI::Property("EnvironmentMap");
			{
				String label;
				Ref<EnvironmentMap> envMap = m_Context->GetEnvironment()->EnvironmentMap;
				if (envMap)
				{
					const FilePath& envPath = envMap->GetFilePath();
					label = envPath.empty() ? "Load Environment Map" : envPath.stem().string();
				}
				else
				{
					label = "Load Environment Map";
				}

				if (ImGui::Button(label.data()))
				{
					FilePath filepath = FileDialogs::OpenFile("EnvironmentMap (*hdr)\0*.hdr\0");
					if (!filepath.empty())
					{
						m_Context->GetEnvironment()->EnvironmentMap = EnvironmentMap::Create(filepath);
					}
				}
				if (ImGui::Button("Delete"))
				{
					m_Context->GetEnvironment()->EnvironmentMap = nullptr;
				}
			}

			const std::string_view resolutions[] = { "256", "512", "1024", "2048", "4096" };
			std::string_view selected = std::to_string(environment->EnvironmentMap->GetResolution()).data();

			UI::Property("Resolution");
			if (UI::ComboBox("##Resolution", resolutions, std::size(resolutions), &selected))
			{
				uint32 resolution = std::atoi(selected.data());
				environment->EnvironmentMap->SetResolution(resolution);
			}

			UI::Property("Ambient Intensity");
			ImGui::SliderFloat("##Ambient Intensity", &environment->AmbientLightIntensity, 0.f, 10.f);

			UI::Property("Environment Map LOD");
			ImGui::SliderFloat("##Environment Map LOD", &environment->EnvironmentMapLOD, 0, ShaderConstants::MAX_SKYBOX_MAP_LOD - 1);

			UI::Property("Exposure");
			ImGui::SliderFloat("##Exposure", &environment->Exposure, 0.001, 10);

			UI::Property("Gamma");
			ImGui::SliderFloat("##Gamma", &environment->Gamma, 0.000, 10);

			UI::EndPropertyTable();
			UI::TreePop();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawMaterialsEditor()
	{
		std::vector<String> materials;
		if(m_SelectionContext.HasComponent<StaticMeshComponent>())
		{
			auto mesh = m_SelectionContext.GetComponent<StaticMeshComponent>().Mesh;
			const auto& subMeshes = mesh->GetAllSubMeshes();
			materials.reserve(subMeshes.size());
			for (uint32 i = 0; i < subMeshes.size(); ++i)
			{
				const String& material = subMeshes[i].MaterialName;
				if (std::find(materials.begin(), materials.end(), material) == materials.end())
				{
					materials.push_back(material);
				}
			}

			if (std::find(materials.begin(), materials.end(), m_ActiveMaterial) == materials.end())
				m_ActiveMaterial = materials[0];
		}

		if (!materials.empty())
		{
			if (m_ActiveMaterial.empty())
				m_ActiveMaterial = materials[0];

			if (UI::BeginPropertyTable())
			{
				UI::Property("Material List");
				UI::ComboBox("##Material List", materials.data(), materials.size(), &m_ActiveMaterial);;

				UI::EndPropertyTable();
			}
			
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });

			bool open = UI::TreeNode("Material");

			ImGui::PopStyleVar(2);

			Ref<Material> material = MaterialManager::Get(m_ActiveMaterial);

			if (open && UI::BeginPropertyTable())
			{
				DrawMaterialProperty(material, "Albedo", "Color", MaterialTexture::ALBEDO_MAP, MaterialUniform::ALBEDO);
				DrawMaterialProperty(material, "Normal Map", "", MaterialTexture::NORMAL_MAP, (MaterialUniform)0);
				DrawMaterialProperty(material, "Roughness", "Roughness", MaterialTexture::ROUGHNESS_MAP, MaterialUniform::ROUGHNESS);
				DrawMaterialProperty(material, "Metalness", "Metalness", MaterialTexture::METALNESS_MAP, MaterialUniform::METALNESS);
				DrawMaterialProperty(material, "Ambient Occlusion", "", MaterialTexture::AMBIENT_OCCLUSION_MAP, (MaterialUniform)0);

				UI::EndPropertyTable();
			}

			if (open)
			{
				UI::TreePop();
				ImGui::Spacing();
			}
		}
	}

	void SceneHierarchyPanel::DrawMaterialProperty(Ref<Material> mat, std::string_view name, std::string_view uniformName, MaterialTexture texType, MaterialUniform uniformType)
	{
		ImGui::PushID(name.data());
		UI::Property(name.data(), 50.f);
		{
			float imageSize = 45.f;

			Ref<Texture2D> albedoMap = mat->Get(texType);
			void* rendererID = albedoMap ? albedoMap->GetRendererID() : Renderer::GetWhiteTexture()->GetRendererID();

			if (ImGui::ImageButton("##MaterialMap", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
			{
				FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
				if (!path.empty())
				{
					albedoMap = Texture2D::Create(path);
					mat->Set(texType, albedoMap);
				}
			}
			ImGui::SameLine();

			bool enableAlbedoMap = mat->IsEnabled(texType);
			ImGui::Checkbox("Enable", &enableAlbedoMap);
			mat->Enable(texType, enableAlbedoMap);

			if (!uniformName.empty())
			{
				ImGui::SameLine();
				if (uniformType == MaterialUniform::ALBEDO)
				{
					Vector3 albedo = mat->Get<Vector3>(uniformType);
					ImGui::ColorEdit3(uniformName.data(), albedo.Data(), ImGuiColorEditFlags_NoInputs);
					mat->Set(uniformType, albedo);

					float emission = mat->Get<float>(MaterialUniform::EMISSION);
					ImGui::DragFloat("Emission", &emission);
					mat->Set(MaterialUniform::EMISSION, emission);
				}
				else
				{
					float uniform = mat->Get<float>(uniformType);
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::SliderFloat(uniformName.data(), &uniform, 0.f, 1.f);
					mat->Set(uniformType, uniform);
				}
			}
		}
		ImGui::PopID();
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		float fullWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 3.f, 3.f });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5.f, 5.f });
		UI::ShiftCursorX(5.f);
		if (ImGui::Button("Edit"))
			m_EditTagComponent = !m_EditTagComponent;

		ImGui::SameLine();

		auto& tag = entity.GetComponent<TagComponent>().Tag;
		if (m_EditTagComponent)
		{
			if (!ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
			UI::TextInput(tag, tag);
			if (ImGui::IsItemDeactivatedAfterEdit())
				m_EditTagComponent = false;
		}
		else 
		{
			UI::PushBoldFont();
			ImGui::Text(tag.data());
			ImGui::PopFont();
		}

		ImGui::SameLine();

		auto& style = ImGui::GetStyle();
		float buttonWidth = ImGui::CalcTextSize("Add Component").x + 2 * style.ItemInnerSpacing.x;
		ImGui::SetCursorPosX(fullWidth - buttonWidth - 5.f);
		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("Add Component");

		if (ImGui::BeginPopup("Add Component"))
		{
			DrawAddComponentEntry<TransformComponent>(entity, "Transform");
			DrawAddComponentEntry<CameraComponent>(entity, "Camera");
			DrawAddComponentEntry<ScriptComponent>(entity, "Script");
			DrawAddComponentEntry<SpriteComponent>(entity, "Sprite");
			DrawAddComponentEntry<CircleComponent>(entity, "Circle");
			DrawAddComponentEntry<Rigidbody2DComponent>(entity, "Rigidbody2D");
			DrawAddComponentEntry<BoxCollider2DComponent>(entity, "BoxCollider2D");
			DrawAddComponentEntry<CircleCollider2DComponent>(entity, "CircleCollider2D");
			DrawAddComponentEntry<StaticMeshComponent>(entity, "StaticMesh");
			DrawAddComponentEntry<DirectionalLightComponent>(entity, "DirectionalLight");
			DrawAddComponentEntry<PointLightComponent>(entity, "PointLight");

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(2);

		ImGui::Spacing();

		DrawComponent<TransformComponent>(entity, "Transform", [](TransformComponent& transform)
			{
				if (UI::BeginPropertyTable())
				{
					DrawVec3Property("Translation", transform.Translation, 0.0f);

					Vector3 degrees = Math::Degrees(transform.Rotation.AsEulerAngles());
					DrawVec3Property("Rotation", degrees, 0.0f);
					transform.Rotation = Math::Radians(degrees);

					DrawVec3Property("Scale", transform.Scale, 1.0f);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<ScriptComponent>(entity, "Script", [entity](ScriptComponent& script)
			{
				if (UI::BeginPropertyTable())
				{
					auto modules = ScriptEngine::GetAvailableModules();

					UI::Property("ScriptName");
					UI::ComboBox("##ScriptName", modules.data(), modules.size(), &script.Name);

					if (!ScriptEngine::ExistsScript(script.Name))
					{
						UI::EndPropertyTable();
						return;
					}

					const ScriptFieldsDescription& fieldsDesc = ScriptEngine::GetFieldsDescription(script.Name);
					ScriptFieldMap& fieldMap = ScriptEngine::GetScriptFieldMap(entity);

					for (const auto& [name, field] : fieldsDesc)
					{
						ImGui::PushID(name.c_str());
						UI::Property(name.data());

						auto& fieldStorage = fieldMap.at(name);
						if (field.Type == ScriptFieldType::Int)
						{
							int data = fieldStorage.GetValue<int>();
							if (ImGui::DragInt("##int", &data))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Float)
						{
							float data = fieldStorage.GetValue<float>();
							if (ImGui::DragFloat("##float", &data))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Bool)
						{
							bool data = fieldStorage.GetValue<bool>();
							if (ImGui::Checkbox("##bool", &data))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector2)
						{
							Vector2 data = fieldStorage.GetValue<Vector2>();
							if (ImGui::DragFloat2("##Vector2", data.Data()))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector3)
						{
							Vector3 data = fieldStorage.GetValue<Vector3>();
							if (ImGui::DragFloat3("##Vector3", data.Data()))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector4)
						{
							Vector4 data = fieldStorage.GetValue<Vector4>();
							if (ImGui::DragFloat4("##Vector4", data.Data()))
								fieldStorage.SetValue(data);
						}

						ImGui::PopID();
					}

					UI::EndPropertyTable();
				}
			});

		DrawComponent<CameraComponent>(entity, "Camera", [](CameraComponent& cameraComponent)
			{
				auto& camera = cameraComponent.Camera;

				static auto typeToStr = [](SceneCamera::ProjectionType type) -> std::string_view
				{
					switch (type)
					{
					case SceneCamera::ProjectionType::Orthographic: return "Orthographic";
					case SceneCamera::ProjectionType::Perspective: return "Perspective";
					}

					return "Invalid";
				};

				static auto strToType = [](std::string_view str) -> SceneCamera::ProjectionType
				{
					if (str == "Orthographic")
						return SceneCamera::ProjectionType::Orthographic;

					if (str == "Perspective")
						return SceneCamera::ProjectionType::Perspective;

					return (SceneCamera::ProjectionType)0;
				};

				if (UI::BeginPropertyTable())
				{
					std::string_view projTypesStrings[] = { "Orthographic", "Perspective" };
					std::string_view currentTypeString = typeToStr(camera.GetProjectionType());

					UI::Property("Projection");
					if(UI::ComboBox("##Projection", projTypesStrings, std::size(projTypesStrings), &currentTypeString))
					{
						camera.SetProjectionType(strToType(currentTypeString));
					}

					if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
					{
						auto perspectiveDesc = camera.GetPerspectiveData();
						bool used = false;

						UI::Property("FOV");
						float degreesFOV = Math::Degrees(perspectiveDesc.VerticalFOV);
						used = ImGui::SliderFloat("##FOV", &degreesFOV, 0.f, 360.f) || used;

						UI::Property("NearClip");
						used = ImGui::SliderFloat("##NearClip", &perspectiveDesc.NearClip, 0.f, 10.f) || used;

						UI::Property("FarClip");
						used = ImGui::SliderFloat("##FarClip", &perspectiveDesc.FarClip, 1000.f, 30000.f) || used;

						if (used)
						{
							perspectiveDesc.VerticalFOV = Math::Radians(degreesFOV);
							camera.SetPerspectiveData(perspectiveDesc);
						}
					}

					else if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
					{
						auto orthoDesc = camera.GetOrthographicData();
						bool used = false;

						UI::Property("Size");
						used = ImGui::DragFloat("##Size", &orthoDesc.Size, 0.1f) || used;

						UI::Property("NearClip");
						used = ImGui::SliderFloat("##NearClip", &orthoDesc.NearClip, -10.f, 0.f) || used;

						UI::Property("FarClip");
						used = ImGui::SliderFloat("##FarClip", &orthoDesc.FarClip, 0.f, 10.f) || used;

						if (used)
						{
							camera.SetOrthographicData(orthoDesc);
						}
					}

					UI::Property("Prmiary");
					ImGui::Checkbox("##Primary", &cameraComponent.Primary);

					UI::Property("FixedAspectRatio");
					ImGui::Checkbox("##FixedAspectRatio", &cameraComponent.FixedAspectRatio);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<SpriteComponent>(entity, "Sprite", [](SpriteComponent& sprite)
			{
				if (UI::BeginPropertyTable())
				{
					UI::Property("Color");
					ImGui::ColorEdit4("##Color", sprite.Color.Data());

					UI::Property("Tiling");
					ImGui::SliderFloat("##Tiling", &sprite.TilingFactor, 1.f, 20.f);

					UI::Property("Texture", 50.f);
					{
						float imageSize = 45.f;

						ImGui::Image(sprite.Texture.GetNativeTexture()->GetRendererID(), { imageSize, imageSize }, { 0, 1 }, { 1, 0 });

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
							{
								std::string_view path = (const char*)payload->Data;
								std::string_view extent = path.substr(path.size() - 4, path.size());
								if (extent == ".png\0")
								{
									sprite.Texture = Texture2D::Create(FilePath(path));
									sprite.Color = LinearColor::White;
								}
								else
								{
									ATN_CORE_ERROR("Invalid Texture format");
								}
							}
							ImGui::EndDragDropTarget();
						}

						ImGui::SameLine();
						ImVec2 cursor = ImGui::GetCursorPos();
						if (ImGui::Button("Browse"))
						{
							FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!path.empty())
							{
								sprite.Texture = Texture2D::Create(path);
								sprite.Color = LinearColor::White;
								ATN_CORE_INFO("Successfuly load Texture from '{0}'", path.string());
							}
							else
							{
								ATN_CORE_ERROR("Invalid filepath to load Texture '{0}'", path.string());
							}
						}

						ImGui::SetCursorPos({ cursor.x, cursor.y + imageSize / 1.8f });
						if (ImGui::Button("Reset"))
						{
							sprite.Texture = Renderer::GetWhiteTexture();
						}
					}

					UI::EndPropertyTable();
				}
			});

		DrawComponent<CircleComponent>(entity, "Circle", [](CircleComponent& circle)
			{
				if (UI::BeginPropertyTable())
				{
					UI::Property("Color");
					ImGui::ColorEdit4("##Color", circle.Color.Data());

					UI::Property("Thickness");
					ImGui::SliderFloat("##Thickness", &circle.Thickness, 0.f, 1.f);

					UI::Property("Fade");
					ImGui::SliderFloat("##Fade", &circle.Fade, 0.f, 1.f);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<Rigidbody2DComponent>(entity, "Rigidbody2D", [](Rigidbody2DComponent& rb2d)
			{
				static auto typeToStr = [](Rigidbody2DComponent::BodyType type) -> std::string_view
				{
					switch (type)
					{
					case Rigidbody2DComponent::BodyType::STATIC: return "Static";
					case Rigidbody2DComponent::BodyType::DYNAMIC: return "Dynamic";
					case Rigidbody2DComponent::BodyType::KINEMATIC: return "Kinematic";
					}

					return "Invalid";
				};

				static auto strToType = [](std::string_view str) -> Rigidbody2DComponent::BodyType
				{
					if (str == "Static")
						return Rigidbody2DComponent::BodyType::STATIC;

					if (str == "Dynamic")
						return Rigidbody2DComponent::BodyType::DYNAMIC;

					if (str == "Kinematic")
						return Rigidbody2DComponent::BodyType::KINEMATIC;

					return (Rigidbody2DComponent::BodyType)0;
				};


				if (UI::BeginPropertyTable())
				{
					std::string_view bodyTypesStrings[] = { "Static", "Dynamic", "Kinematic"};
					std::string_view currentBodyType = typeToStr(rb2d.Type);

					UI::Property("BodyType");
					if (UI::ComboBox("##BodyType", bodyTypesStrings, std::size(bodyTypesStrings), &currentBodyType))
					{
						rb2d.Type = strToType(currentBodyType);
					}

					UI::Property("Fixed Rotation");
					ImGui::Checkbox("##FixedRotation", &rb2d.FixedRotation);
					
					UI::EndPropertyTable();
				}
			});


		DrawComponent<BoxCollider2DComponent>(entity, "BoxCollider2D", [](BoxCollider2DComponent& bc2d)
			{
				if (UI::BeginPropertyTable())
				{
					UI::Property("Offset");
					ImGui::DragFloat2("##Offset", bc2d.Offset.Data(), 0.1f);

					UI::Property("Size");
					ImGui::DragFloat2("##Size", bc2d.Size.Data(), 0.1f);

					UI::Property("Density");
					ImGui::SliderFloat("##Density", &bc2d.Density, 0.f, 1.f);

					UI::Property("Friction");
					ImGui::SliderFloat("##Friction", &bc2d.Friction, 0.f, 1.f);

					UI::Property("Restitution");
					ImGui::SliderFloat("##Restitution", &bc2d.Restitution, 0.f, 1.f);

					UI::Property("RestitutionThreshold");
					ImGui::SliderFloat("##RestitutionThreshold", &bc2d.RestitutionThreshold, 0.f, 1.f);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<CircleCollider2DComponent>(entity, "CircleCollider2D", [](CircleCollider2DComponent& cc2d)
			{
				if (UI::BeginPropertyTable())
				{
					UI::Property("Offset");
					ImGui::DragFloat2("##Offset", cc2d.Offset.Data(), 0.1f);

					UI::Property("Radius");
					ImGui::DragFloat("##Radius", &cc2d.Radius, 0.1f);

					UI::Property("Density");
					ImGui::SliderFloat("##Density", &cc2d.Density, 0.f, 1.f);

					UI::Property("Friction");
					ImGui::SliderFloat("##Friction", &cc2d.Friction, 0.f, 1.f);

					UI::Property("Restitution");
					ImGui::SliderFloat("##Restitution", &cc2d.Restitution, 0.f, 1.f);

					UI::Property("RestitutionThreshold");
					ImGui::SliderFloat("##RestitutionThreshold", &cc2d.RestitutionThreshold, 0.f, 1.f);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<StaticMeshComponent>(entity, "StaticMesh", [this, entity](StaticMeshComponent& meshComponent)
			{
				if (UI::BeginPropertyTable())
				{
					String meshFilename = meshComponent.Mesh->GetFilePath().filename().string();

					UI::Property("Mesh");
					ImGui::Text(meshFilename.c_str());

					UI::Property("Hide");
					ImGui::Checkbox("##Hide", &meshComponent.Hide);

					UI::EndPropertyTable();

					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
					if (animator)
					{
						if (UI::TreeNode("Animations") && UI::BeginPropertyTable())
						{
							Ref<Animation> active = animator->GetCurrentAnimation();
							active = active ? active : (animator->GetAllAnimations().size() > 0 ? animator->GetAllAnimations()[0] : nullptr);

							std::string_view selectedAnim = active ? active->GetName().c_str() : "";

							const auto& animations = animator->GetAllAnimations();
							std::vector<std::string_view> animNames(animations.size());
							for (uint32 i = 0; i < animations.size(); ++i)
								animNames[i] = animations[i]->GetName();

							UI::Property("Animation List");
							if (UI::ComboBox("##Animation List", animNames.data(), animNames.size(), &selectedAnim))
							{
								uint32 index = std::distance(animNames.begin(), std::find(animNames.begin(), animNames.end(), selectedAnim));
								Ref<Animation> anim = animations[index];
								animator->PlayAnimation(anim);
							}

							UI::Property("Play");
							{ 
								bool playNow = active == animator->GetCurrentAnimation();
								bool check = playNow;
								ImGui::Checkbox("##Play", &check); 

								if (check && !playNow)
									animator->PlayAnimation(active);
								else if (!check && playNow)
									animator->StopAnimation();

								if (check)
								{
									Ref<Animation> anim = animator->GetCurrentAnimation();
									uint32 ticks = anim->GetTicksPerSecond();
									float animTime = animator->GetAnimationTime() / (float)ticks;
									ImGui::SameLine();
									ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
									ImGui::SliderFloat("##Duration", &animTime, 0, (anim->GetDuration() - 1) / (float)ticks, nullptr, ImGuiSliderFlags_NoInput);
									animator->SetAnimationTime(animTime * (float)ticks);
								}
							}

							UI::EndPropertyTable();

							UI::TreePop();
							ImGui::Spacing();
						}
					}
				}
			});

		DrawComponent<DirectionalLightComponent>(entity, "DirectionalLight", [](DirectionalLightComponent& lightComponent)
			{
				if (UI::BeginPropertyTable())
				{
					UI::Property("Color");
					ImGui::ColorEdit3("##Color", lightComponent.Color.Data());

					UI::Property("Intensity");
					ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);

					UI::EndPropertyTable();
				}
			});

		DrawComponent<PointLightComponent>(entity, "PointLight", [](PointLightComponent& lightComponent)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginPropertyTable())
				{
					UI::Property("Color");
					ImGui::ColorEdit3("##Color", lightComponent.Color.Data());

					UI::Property("Intensity");
					ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);

					UI::Property("Radius");
					ImGui::DragFloat("##Radius", &lightComponent.Radius, 2.5f, 0.f, 10000.f);

					UI::Property("FallOff");
					ImGui::DragFloat("##FallOff", &lightComponent.FallOff, 0.1f, 0.f, 100.f);

					UI::EndPropertyTable();
				}
			});
	}
}
