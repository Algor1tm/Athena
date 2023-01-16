#include "SceneHierarchyPanel.h"

#include "Athena/Core/PlatformUtils.h"
#include "Athena/Input/Input.h"

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
		ImGui::Begin("Properties");
		ImGui::PopStyleVar();

		if (m_SelectionContext)
		{
			DrawAllComponents(m_SelectionContext);
		}

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
			DrawAddComponentEntry<StaticMeshComponent>(entity, "StaticMeshComponent");

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
							{ return ImGui::SliderFloat("##FarClip", &perspectiveDesc.FarClip, 10.f, 10000.f); }) || used;

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
										sprite.Texture = Texture2D::Create(String(path));
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
								String filepath = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
								if (!filepath.empty())
								{
									sprite.Texture = Texture2D::Create(filepath);
									sprite.Color = LinearColor::White;
									ATN_CORE_INFO("Successfuly load Texture from '{0}'", filepath.data());
								}
								else
								{
									ATN_CORE_ERROR("Invalid filepath to load Texture '{0}'", filepath.data());
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

		DrawComponent<StaticMeshComponent>(entity, "StaticMeshComponent", [this, entity](StaticMeshComponent& meshComponent)
			{
				float height = ImGui::GetFrameHeight();

				if (UI::BeginDrawControllers())
				{
					if (UI::DrawController("Mesh", height, [&meshComponent]()
						{ return ImGui::BeginCombo("##BodyType", meshComponent.Mesh->GetName().data()); }))
					{
						const Filepath meshDir = "Assets/Meshes";
						const Filepath meshFilename = meshComponent.Mesh->GetFilepath().filename();

						for (const auto& dirEntry : std::filesystem::directory_iterator(meshDir))
						{
							if (!dirEntry.is_directory())
							{
								const auto& filename = dirEntry.path().filename();
								bool open = meshFilename == filename;
								UI::Selectable(filename.string(), &open, [&meshComponent, &meshDir, &filename, this]()
									{
										meshComponent.Mesh = StaticMesh::Create(meshDir / filename, m_Context);
									});
							}
						}

						ImGui::EndCombo();
					}

					UI::EndDrawControllers();
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4, 6 });

				float lineHeight = ImGui::GetFrameHeight();

				bool open = UI::BeginTreeNode("Material");

				ImGui::PopStyleVar(2);

				Ref<Material> material = m_Context->GetMaterial(meshComponent.Mesh->GetMaterialIndex());
				auto& matDesc = material->GetDescription();

				if (UI::BeginDrawControllers())
				{
					void* whiteTexRendererID = Texture2D::WhiteTexture()->GetRendererID();

					UI::DrawController("Albedo", 50.f, [&matDesc, whiteTexRendererID]
						{ 
							float imageSize = 45.f;

							void* rendererID;
							if (matDesc.AlbedoTexture)
								rendererID = matDesc.AlbedoTexture->GetRendererID();
							else
								rendererID = whiteTexRendererID;

							ImGui::Image(rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 });
							ImGui::SameLine();
							ImGui::Checkbox("Use", &matDesc.UseAlbedoTexture);
							ImGui::SameLine();
							ImGui::ColorEdit3("Color", matDesc.Albedo.Data(), ImGuiColorEditFlags_NoInputs);
							return true;
						});

					UI::DrawController("Normals", 50.f, [&matDesc, whiteTexRendererID]
						{
							float imageSize = 45.f;

							void* rendererID;
							if (matDesc.NormalMap)
								rendererID = matDesc.NormalMap->GetRendererID();
							else
								rendererID = whiteTexRendererID;

							ImGui::Image(rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 });
							ImGui::SameLine();
							ImGui::Checkbox("Use", &matDesc.UseNormalMap);
							return true;
						});

					UI::DrawController("Roughness", 50.f, [&matDesc, whiteTexRendererID]
						{
							float imageSize = 45.f;

							void* rendererID;
							if (matDesc.RoughnessMap)
								rendererID = matDesc.RoughnessMap->GetRendererID();
							else
								rendererID = whiteTexRendererID;

							ImGui::Image(rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 });
							ImGui::SameLine();
							ImGui::Checkbox("Use", &matDesc.UseRoughnessMap);
							ImGui::SameLine();
							ImGui::SliderFloat("##Roughness", &matDesc.Roughness, 0.f, 1.f);
							return true;
						});

					UI::DrawController("Metalness", 50.f, [&matDesc, whiteTexRendererID]
						{
							float imageSize = 45.f;

							void* rendererID;
							if (matDesc.MetalnessMap)
								rendererID = matDesc.MetalnessMap->GetRendererID();
							else
								rendererID = whiteTexRendererID;

							ImGui::Image(rendererID, { imageSize, imageSize }, { 0, 1 }, { 1, 0 });
							ImGui::SameLine();
							ImGui::Checkbox("Use", &matDesc.UseMetalnessMap);
							ImGui::SameLine();
							ImGui::SliderFloat("##Metalness", &matDesc.Metalness, 0.f, 1.f);
							return true;
						});
					UI::EndDrawControllers();
				}

				if (open)
				{
					UI::EndTreeNode();
					ImGui::Spacing();
				}
			});
	}
}
