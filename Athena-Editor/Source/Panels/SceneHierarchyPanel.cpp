#include "SceneHierarchyPanel.h"

#include "Athena/Core/PlatformUtils.h"

#include "Athena/Input/Input.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Scene/Components.h"

#include "Athena/Scripting/PublicScriptEngine.h"

#include "UI/Widgets.h"

#include <ImGui/imgui.h>


namespace Athena
{
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
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		if (ImGui::Begin("Environment"))
		{
			DrawEnvironment(m_Context->GetEnvironment());
		}

		ImGui::PopStyleVar();
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		if (ImGui::Begin("Materials"))
		{
			DrawMaterials();
		}

		ImGui::PopStyleVar();
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetDarkColor());
		ImGui::Begin("Scene Hierarchy");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
		ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
		ImGui::PushStyleColor(ImGuiCol_Header, color);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

		m_Context->m_Registry.each([=](auto entityID)
			{
				Entity entity(entityID, m_Context.get());

				DrawEntityNode(entity);
			});
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

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		if (ImGui::Begin("Properties"))
		{
			if (m_SelectionContext)
			{
				DrawAllComponents(m_SelectionContext);
			}
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0);
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		flags |= ImGuiTreeNodeFlags_FramePadding;

		bool selectedEntity = m_SelectionContext == entity;
		if(selectedEntity)
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive]);

		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (selectedEntity)
			ImGui::PopStyleColor();

		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened)
			ImGui::TreePop();

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	void SceneHierarchyPanel::DrawEnvironment(const Ref<Environment>& environment)
	{
		float height = ImGui::GetFrameHeight();

		if (UI::BeginTreeNode("Environment"))
		{
			if (UI::BeginDrawControllers())
			{
				UI::DrawController("Skybox", height, [this]() 
					{
						String label;
						Ref<Skybox> skybox = m_Context->GetEnvironment()->Skybox;
						if (skybox)
						{
							const FilePath& envPath = skybox->GetFilePath();
							label = envPath.empty() ? "Load Environment Map" : envPath.stem().string();
						}
						else
						{
							label = "Load Environment Map";
						}

						if (ImGui::Button(label.data()))
						{
							FilePath filepath = FileDialogs::OpenFile("Skybox (*hdr)\0*.hdr\0");
							if (!filepath.empty())
							{
								Ref<Skybox> skybox = Skybox::Create(filepath);
								m_Context->GetEnvironment()->Skybox = skybox;
							}
							return true;
						}
						if (ImGui::Button("Delete"))
						{
							m_Context->GetEnvironment()->Skybox = nullptr;
							return true;
						}

						return false;
					});
				UI::DrawController("Skybox LOD", height, [&environment]() {return ImGui::SliderFloat("##SkyboxLOD", &environment->SkyboxLOD, 0, ShaderConstants::MAX_SKYBOX_MAP_LOD - 1); });
				UI::DrawController("Exposure", height, [&environment]() {return ImGui::SliderFloat("##Exposure", &environment->Exposure, 0.001, 7); });

				UI::EndDrawControllers();
			}

			UI::EndTreeNode();
		}
	}

	void SceneHierarchyPanel::DrawMaterials()
	{
		float height = ImGui::GetFrameHeight();

		std::vector<String> materials;
		if (m_SelectionContext)
		{
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
		}
		else
		{
			m_ActiveMaterial.clear();
		}

		if (!materials.empty())
		{
			if (m_ActiveMaterial.empty())
				m_ActiveMaterial = materials[0];

			if (UI::BeginDrawControllers())
			{
				if (UI::DrawController("Material List", height, [this]()
					{ return ImGui::BeginCombo("##Material List", m_ActiveMaterial.data()); }))
				{
					for (const auto& material : materials)
					{
						bool open = material == m_ActiveMaterial;
						UI::Selectable(material, &open, [this, material]() 
							{
								m_ActiveMaterial = material;
							});
					}

					ImGui::EndCombo();
				}
				UI::EndDrawControllers();
			}
			
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });

			bool open = UI::BeginTreeNode("Material");

			ImGui::PopStyleVar(2);

			Ref<Material> material = MaterialManager::GetMaterial(m_ActiveMaterial);
			auto& matDesc = material->GetDescription();

			if (open && UI::BeginDrawControllers())
			{
				void* whiteTexRendererID = Texture2D::WhiteTexture()->GetRendererID();

				ImGui::PushID("Albedo");
				UI::DrawController("Albedo", 50.f, [&matDesc, whiteTexRendererID]
					{
						float imageSize = 45.f;

						void* rendererID;
						if (matDesc.AlbedoMap)
							rendererID = matDesc.AlbedoMap->GetRendererID();
						else
							rendererID = whiteTexRendererID;

						if (ImGui::ImageButton("##AlbedoMap", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
						{
							FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!path.empty())
							{
								matDesc.AlbedoMap = Texture2D::Create(path);
								matDesc.UseAlbedoMap = true;
							}
						}
						ImGui::SameLine();
						ImGui::Checkbox("Use", &matDesc.UseAlbedoMap);
						ImGui::SameLine();
						ImGui::ColorEdit3("Color", matDesc.Albedo.Data(), ImGuiColorEditFlags_NoInputs);
						return true;
					});
				ImGui::PopID();
				ImGui::PushID("Normal");
				UI::DrawController("Normals", 50.f, [&matDesc, whiteTexRendererID]
					{
						float imageSize = 45.f;

						void* rendererID;
						if (matDesc.NormalMap)
							rendererID = matDesc.NormalMap->GetRendererID();
						else
							rendererID = whiteTexRendererID;

						if (ImGui::ImageButton("##Normals", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
						{
							FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!path.empty())
							{
								matDesc.NormalMap = Texture2D::Create(path);
								matDesc.UseNormalMap = true;
							}
						}
						ImGui::SameLine();
						ImGui::Checkbox("Use", &matDesc.UseNormalMap);
						return true;
					});
				ImGui::PopID();
				ImGui::PushID("Roughness");
				UI::DrawController("Roughness", 50.f, [&matDesc, whiteTexRendererID]
					{
						float imageSize = 45.f;

						void* rendererID;
						if (matDesc.RoughnessMap)
							rendererID = matDesc.RoughnessMap->GetRendererID();
						else
							rendererID = whiteTexRendererID;

						if (ImGui::ImageButton("##RoughnessMap", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
						{
							FilePath filepath = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!filepath.empty())
							{
								matDesc.RoughnessMap = Texture2D::Create(filepath);
								matDesc.UseRoughnessMap = true;
							}
						}
						ImGui::SameLine();
						ImGui::Checkbox("Use", &matDesc.UseRoughnessMap);
						ImGui::SameLine();

						ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
						ImGui::SliderFloat("##Roughness", &matDesc.Roughness, 0.f, 1.f);
						return true;
					});
				ImGui::PopID();
				ImGui::PushID("Metalness");
				UI::DrawController("Metalness", 50.f, [&matDesc, whiteTexRendererID]
					{
						float imageSize = 45.f;

						void* rendererID;
						if (matDesc.MetalnessMap)
							rendererID = matDesc.MetalnessMap->GetRendererID();
						else
							rendererID = whiteTexRendererID;

						if (ImGui::ImageButton("##MetalnessMap", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
						{
							FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!path.empty())
							{
								matDesc.MetalnessMap = Texture2D::Create(path);
								matDesc.UseMetalnessMap = true;
							}
						}
						ImGui::SameLine();
						ImGui::Checkbox("Use", &matDesc.UseMetalnessMap);
						ImGui::SameLine();
						ImGui::SliderFloat("##Metalness", &matDesc.Metalness, 0.f, 1.f);
						return true;
					});
				ImGui::PopID();
				ImGui::PushID("AmbientOcclusion");
				UI::DrawController("Ambient Occlusion", 50.f, [&matDesc, whiteTexRendererID]
					{
						float imageSize = 45.f;

						void* rendererID;
						if (matDesc.AmbientOcclusionMap)
							rendererID = matDesc.AmbientOcclusionMap->GetRendererID();
						else
							rendererID = whiteTexRendererID;

						if (ImGui::ImageButton("##AmbientOcclusionMap", rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 }))
						{
							FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
							if (!path.empty())
							{
								matDesc.AmbientOcclusionMap = Texture2D::Create(path);
								matDesc.UseAmbientOcclusionMap = true;
							}
						}
						ImGui::SameLine();
						ImGui::Checkbox("Use", &matDesc.UseAmbientOcclusionMap);
						ImGui::SameLine();
						ImGui::SliderFloat("##AmbientOcclusion", &matDesc.AmbientOcclusion, 0.f, 1.f);
						return true;
					});
				ImGui::PopID();
				UI::EndDrawControllers();
			}

			if (open)
			{
				UI::EndTreeNode();
				ImGui::Spacing();
			}
		}
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
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawVec3Controller("Translation", transform.Translation, 0.0f, height);
					Vector3 degrees = Math::Degrees(transform.Rotation);
					UI::DrawVec3Controller("Rotation", degrees, 0.0f, height);
					transform.Rotation = Math::Radians(degrees);
					UI::DrawVec3Controller("Scale", transform.Scale, 1.0f, height);

					UI::EndDrawControllers();
				}
			});

		DrawComponent<ScriptComponent>(entity, "Script", [entity](ScriptComponent& script)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					if (UI::DrawController("ScriptName", height, [&script]()
						{ return ImGui::BeginCombo("##Scripts", script.Name.data()); }))
					{
						auto modules = PublicScriptEngine::GetAvailableModules();
						auto currentName = std::string_view(script.Name.data());

						for (const auto& moduleName : modules)
						{
							bool isSelected = currentName == moduleName;

							UI::Selectable(moduleName, &isSelected, [&script, &moduleName]() 
								{
									script.Name = moduleName;
								});

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					if (script.Name.empty())
					{
						UI::EndDrawControllers();
						return;
					}

					const ScriptFieldsDescription& fieldsDesc = PublicScriptEngine::GetFieldsDescription(script.Name);
					ScriptFieldMap& fieldMap = PublicScriptEngine::GetScriptFieldMap(entity);

					if (!fieldsDesc.empty() && fieldMap.empty())	// TODO: Move to ScriptEngine
					{												
						for (const auto& [name, field] : fieldsDesc)	// fill FieldMap
						{
							ScriptFieldStorage& storage = fieldMap[name];
							storage = field.Storage;
						}
					}

					for (const auto& [name, field] : fieldsDesc)
					{
						auto& fieldStorage = fieldMap[name];
						if (field.Type == ScriptFieldType::Int)
						{
							int data = fieldStorage.GetValue<int>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::DragInt("##int", &data); }))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Float)
						{
							float data = fieldStorage.GetValue<float>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::DragFloat("##float", &data); }))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Bool)
						{
							bool data = fieldStorage.GetValue<bool>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::Checkbox("##bool", &data); }))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector2)
						{
							Vector2 data = fieldStorage.GetValue<Vector2>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::DragFloat2("##Vector2", data.Data()); }))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector3)
						{
							Vector3 data = fieldStorage.GetValue<Vector3>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::DragFloat3("##Vector3", data.Data()); }))
								fieldStorage.SetValue(data);
						}
						else if (field.Type == ScriptFieldType::Vector4)
						{
							Vector4 data = fieldStorage.GetValue<Vector4>();
							if (UI::DrawController(name.data(), height, [&data]() { return ImGui::DragFloat4("##Vector4", data.Data()); }))
								fieldStorage.SetValue(data);
						}
					}

					UI::EndDrawControllers();
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

				std::string_view currentProjectionType = typeToStr(camera.GetProjectionType());

				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					if (UI::DrawController("Projection", height, [&currentProjectionType]()
						{ return ImGui::BeginCombo("##Projection", currentProjectionType.data()); }))
					{
						for (int i = 0; i < 2; ++i)
						{
							const std::string_view typeStr = typeToStr((SceneCamera::ProjectionType)i);
							bool isSelected = currentProjectionType == typeStr;

							UI::Selectable(typeStr, &isSelected, [&currentProjectionType, typeStr, &camera, i]()
								{
									currentProjectionType = typeStr;
									camera.SetProjectionType((SceneCamera::ProjectionType)i);
								});

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
					{
						auto perspectiveDesc = camera.GetPerspectiveData();
						bool used = false;

						float degreesFOV = Math::Degrees(perspectiveDesc.VerticalFOV);
						used = UI::DrawController("FOV", height, [&degreesFOV]()
							{ return ImGui::SliderFloat("##FOV", &degreesFOV, 0.f, 360.f); }) || used;
						used = UI::DrawController("NearClip", height, [&perspectiveDesc]()
							{ return ImGui::SliderFloat("##NearClip", &perspectiveDesc.NearClip, 0.f, 10.f); }) || used;
						used = UI::DrawController("FarClip", height, [&perspectiveDesc]()
							{ return ImGui::SliderFloat("##FarClip", &perspectiveDesc.FarClip, 1000.f, 30000.f); }) || used;

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

						used = UI::DrawController("Size", height, [&orthoDesc]()
							{ return ImGui::DragFloat("##Size", &orthoDesc.Size, 0.1f); }) || used;
						used = UI::DrawController("NearClip", height, [&orthoDesc]()
							{ return ImGui::SliderFloat("##NearClip", &orthoDesc.NearClip, -10.f, 0.f); }) || used;
						used = UI::DrawController("FarClip", height, [&orthoDesc]()
							{ return ImGui::SliderFloat("##FarClip", &orthoDesc.FarClip, 0.f, 10.f); }) || used;

						if (used)
							camera.SetOrthographicData(orthoDesc);
					}

					UI::DrawController("Primary", height, [&cameraComponent]()
						{ return ImGui::Checkbox("##Primary", &cameraComponent.Primary); });
					UI::DrawController("FixedAspectRatio", height, [&cameraComponent]()
						{ return ImGui::Checkbox("##FixedAspectRatio", &cameraComponent.FixedAspectRatio); });

					UI::EndDrawControllers();
				}
			});

		DrawComponent<SpriteComponent>(entity, "Sprite", [](SpriteComponent& sprite)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Color", height, [&sprite]() { return ImGui::ColorEdit4("##Color", sprite.Color.Data()); });
					UI::DrawController("Tiling", height, [&sprite]() { return ImGui::SliderFloat("##Tiling", &sprite.TilingFactor, 1.f, 20.f); });

					UI::DrawController("Texture", 50.f, [&sprite]()
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

							ImGui::SetCursorPos({cursor.x, cursor.y + imageSize / 1.8f});
							if (ImGui::Button("Reset"))
							{
								sprite.Texture = Texture2D::WhiteTexture();
							}

							return false;
						});

					UI::EndDrawControllers();
				}
			});

		DrawComponent<CircleComponent>(entity, "Circle", [](CircleComponent& circle)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Color", height, [&circle]() { return ImGui::ColorEdit4("##Color", circle.Color.Data()); });
					UI::DrawController("Thickness", height, [&circle]() { return ImGui::SliderFloat("##Thickness", &circle.Thickness, 0.f, 1.f); });
					UI::DrawController("Fade", height, [&circle]() { return ImGui::SliderFloat("##Fade", &circle.Fade, 0.f, 1.f); });

					UI::EndDrawControllers();
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

				std::string_view currentBodyType = typeToStr(rb2d.Type);

				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					if (UI::DrawController("BodyType", height, [&currentBodyType]()
						{ return ImGui::BeginCombo("##BodyType", currentBodyType.data()); }))
					{
						for (int i = 0; i < 3; ++i)
						{
							const std::string_view typeStr = typeToStr((Rigidbody2DComponent::BodyType)i);
							bool isSelected = currentBodyType == typeStr;

							UI::Selectable(typeStr, &isSelected, [&currentBodyType, typeStr, &rb2d, i]()
								{
										currentBodyType = typeStr;
										rb2d.Type = (Rigidbody2DComponent::BodyType)i;
								});

							if (isSelected)
								ImGui::SetItemDefaultFocus();
						}

						ImGui::EndCombo();
					}

					UI::DrawController("Fixed Rotation", height, [&rb2d] { return ImGui::Checkbox("##FixedRotation", &rb2d.FixedRotation); });

					UI::EndDrawControllers();
				}
			});


		DrawComponent<BoxCollider2DComponent>(entity, "BoxCollider2D", [](BoxCollider2DComponent& bc2d)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Offset", height, [&bc2d]() { return ImGui::DragFloat2("##Offset", bc2d.Offset.Data(), 0.1f); });
					UI::DrawController("Size", height, [&bc2d]() { return ImGui::DragFloat2("##Size", bc2d.Size.Data(), 0.1f); });

					UI::DrawController("Density", height, [&bc2d]() { return ImGui::SliderFloat("##Density", &bc2d.Density, 0.f, 1.f); });
					UI::DrawController("Friction", height, [&bc2d]() { return ImGui::SliderFloat("##Friction", &bc2d.Friction, 0.f, 1.f); });
					UI::DrawController("Restitution", height, [&bc2d]() { return ImGui::SliderFloat("##Restitution", &bc2d.Restitution, 0.f, 1.f); });
					UI::DrawController("RestitutionThreshold", height, [&bc2d]() { return ImGui::SliderFloat("##RestitutionThreshold", &bc2d.RestitutionThreshold, 0.f, 1.f); });

					UI::EndDrawControllers();
				}
			});

		DrawComponent<CircleCollider2DComponent>(entity, "CircleCollider2D", [](CircleCollider2DComponent& cc2d)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Offset", height, [&cc2d]() { return ImGui::DragFloat2("##Offset", cc2d.Offset.Data(), 0.1f); });
					UI::DrawController("Radius", height, [&cc2d]() { return ImGui::DragFloat("##Radius", &cc2d.Radius, 0.1f); });

					UI::DrawController("Density", height, [&cc2d]() { return ImGui::SliderFloat("##Density", &cc2d.Density, 0.f, 1.f); });
					UI::DrawController("Friction", height, [&cc2d]() { return ImGui::SliderFloat("##Friction", &cc2d.Friction, 0.f, 1.f); });
					UI::DrawController("Restitution", height, [&cc2d]() { return ImGui::SliderFloat("##Restitution", &cc2d.Restitution, 0.f, 1.f); });
					UI::DrawController("RestitutionThreshold", height, [&cc2d]() { return ImGui::SliderFloat("##RestitutionThreshold", &cc2d.RestitutionThreshold, 0.f, 1.f); });

					UI::EndDrawControllers();
				}
			});

		DrawComponent<StaticMeshComponent>(entity, "StaticMesh", [this, entity](StaticMeshComponent& meshComponent)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					String meshFilename = meshComponent.Mesh->GetFilePath().filename().string();
					if (UI::DrawController("Mesh", height, [&meshComponent, &meshFilename]()
						{ return ImGui::BeginCombo("##Mesh", meshFilename.data()); }))
					{
						for (const auto& dirEntry : std::filesystem::directory_iterator("Assets/Meshes"))
						{
							if (!dirEntry.is_directory())
							{
								const auto& filename = dirEntry.path().filename().string();
								bool isSelected = filename == meshFilename;
								UI::Selectable(filename, &isSelected, [&meshComponent, &dirEntry]()
									{
										meshComponent.Mesh = StaticMesh::Create(dirEntry.path());
										meshComponent.Hide = false;
									});
							}
						}

						ImGui::EndCombo();
					}

					UI::DrawController("Hide", height, [&meshComponent]() { return ImGui::Checkbox("##Hide", &meshComponent.Hide); });

					UI::EndDrawControllers();

					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
					if (animator)
					{
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 3, 5 });
						bool open = UI::BeginTreeNode("Animations");
						ImGui::PopStyleVar(2);

						if (open && UI::BeginDrawControllers())
						{
							Ref<Animation> active = animator->GetCurrentAnimation();
							active = active ? active : animator->GetAllAnimations()[0];
							const String& preview = active->GetName();

							if (UI::DrawController("Animation List", height, [&meshComponent, preview]()
								{ return ImGui::BeginCombo("##Animation List", preview.data()); }))
							{
								const auto& animations = animator->GetAllAnimations();
								for (uint32 i = 0; i < animations.size(); ++i)
								{
									Ref<Animation> anim = animations[i];
									const String& name = anim->GetName();
									bool open = name == preview;
									UI::Selectable(name, &open, [animator, anim]()
										{
											animator->PlayAnimation(anim);
										});
								}

								ImGui::EndCombo();
							}

							UI::DrawController("Play", height, [animator, preview, active]()
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

									return check;
								});

							UI::EndDrawControllers();

							UI::EndTreeNode();
							ImGui::Spacing();
						}
					}
				}
			});

		DrawComponent<DirectionalLightComponent>(entity, "DirectionalLight", [](DirectionalLightComponent& lightComponent)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Color", height, [&lightComponent]() { return ImGui::ColorEdit3("##Color", lightComponent.Color.Data()); });
					UI::DrawController("Direction", height, [&lightComponent]() { return ImGui::DragFloat3("##Direction", lightComponent.Direction.Data(), 0.05f); });
					UI::DrawController("Intensity", height, [&lightComponent]() { return ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f); });

					UI::EndDrawControllers();
				}
			});

		DrawComponent<PointLightComponent>(entity, "PointLight", [](PointLightComponent& lightComponent)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					UI::DrawController("Color", height, [&lightComponent]() { return ImGui::ColorEdit3("##Color", lightComponent.Color.Data()); });
					UI::DrawController("Intensity", height, [&lightComponent]() { return ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 1.f, 0.f, 10000.f);  });

					UI::DrawController("Radius", height, [&lightComponent]() {  return ImGui::DragFloat("##Radius", &lightComponent.Radius, 5.f, 0.f, 10000.f); });

					UI::DrawController("FallOff", height, [&lightComponent]() {  return ImGui::DragFloat("##FallOff", &lightComponent.FallOff, 0.1f, 0.f, 100.f);  });

					UI::EndDrawControllers();
				}
			});
	}
}
