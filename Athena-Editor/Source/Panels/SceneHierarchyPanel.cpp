#include "SceneHierarchyPanel.h"

#include "Athena/Core/PlatformUtils.h"

#include "Athena/Input/Input.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"

#include "Athena/Scene/Components.h"

#include "Athena/Scripting/ScriptEngine.h"

#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"

#include <ImGui/imgui.h>


namespace Athena
{
	void DrawVec3Property(std::string_view label, Vector3& values, float defaultValues)
	{
		UI::PropertyRow(label, ImGui::GetFrameHeight());

		ImGui::PushID(label.data());

		float full_width = ImGui::GetContentRegionAvail().x - 15.f;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });

		float buttonWidth = ImGui::GetFrameHeight();
		ImVec2 buttonSize = { buttonWidth, buttonWidth };

		float dragWidth = (full_width - 3 * buttonWidth) / 3.f;

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.8f, 0.1f, 0.15f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.9f, 0.2f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.8f, 0.1f, 0.15f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("X", buttonSize))
			values.x = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##X", &values.x, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.7f, 0.2f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.3f, 0.8f, 0.3f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.2f, 0.7f, 0.2f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("Y", buttonSize))
			values.y = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Y", &values.y, 0.07f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PopStyleColor(3);

		ImGui::PushStyleColor(ImGuiCol_Button, { 0.1f, 0.25f, 0.8f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 0.2f, 0.35f, 0.9f, 1.f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 0.1f, 0.25f, 0.8f, 1.f });

		UI::PushFont(UI::Fonts::Bold);
		if (ImGui::Button("Z", buttonSize))
			values.z = defaultValues;
		UI::PopFont();

		ImGui::SameLine();
		ImGui::PushItemWidth(dragWidth);
		ImGui::DragFloat("##Z", &values.z, 0.7f, 0.f, 0.f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleColor(3);

		ImGui::PopStyleVar();

		ImGui::PopID();
	}


	SceneHierarchyPanel::SceneHierarchyPanel(std::string_view name, const Ref<EditorContext>& context)
		: Panel(name, context)
	{
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Materials Editor");
		if (m_EditorCtx.SelectedEntity)
		{
			DrawMaterialsEditor();
		}
		ImGui::PopStyleVar();
		ImGui::End();

		DrawEntitiesHierarchy();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 5.f });
		ImGui::Begin("Properties");
		if (m_EditorCtx.SelectedEntity)
		{
			DrawAllComponents(m_EditorCtx.SelectedEntity);
		}
		ImGui::PopStyleVar();
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntitiesHierarchy()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.f, 0.f });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, UI::GetTheme().BackgroundDark);
		ImGui::Begin("Scene Hierarchy");
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
		ImVec4& color = ImGui::GetStyle().Colors[ImGuiCol_ResizeGripActive];
		ImGui::PushStyleColor(ImGuiCol_Header, color);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, color);

		const auto& view = m_EditorCtx.ActiveScene->GetAllEntitiesWith<IDComponent>();
		for (auto entt : view)
		{
			Entity entity = Entity(entt, m_EditorCtx.ActiveScene.Raw());
			if (!entity.HasComponent<ParentComponent>())
			{
				DrawEntityNode(entity);
			}
		}

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		UI::InvisibleItem("DragDropRegion", ImGui::GetContentRegionAvail());
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SCENE_HIERARCHY_ENTITY", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
			{
				Entity payloadEntity = *(Entity*)payload->Data;
				m_EditorCtx.ActiveScene->MakeOrphan(payloadEntity);
			}

			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			m_EditorCtx.SelectedEntity = {};

		if (ImGui::BeginPopupContextItem("Entity Hierarchy Settings", ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Entity"))
			{
				Entity created = m_EditorCtx.ActiveScene->CreateEntity();
				m_EditorCtx.SelectedEntity = created;
			}

			ImGui::EndPopup();
		}
		ImGui::End();
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		const auto& tag = entity.GetComponent<TagComponent>().Tag;
		bool selected = m_EditorCtx.SelectedEntity == entity;
		bool hasChildren = entity.HasComponent<ChildComponent>();
		bool hasParent = entity.HasComponent<ParentComponent>();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_FramePadding |
			ImGuiTreeNodeFlags_DefaultOpen;

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
			m_EditorCtx.SelectedEntity = entity;
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
					m_EditorCtx.ActiveScene->MakeRelationship(entity, payloadEntity);
				}
			}

			ImGui::EndDragDropTarget();
		}

		Entity selectedEntity = m_EditorCtx.SelectedEntity;
		bool entityDeleted = false;
		// If there are entities with the same tag in the scene, this won`t work correct
		if (ImGui::BeginPopupContextItem(entity.GetComponent<TagComponent>().Tag.c_str(), ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			if (hasParent && ImGui::MenuItem("Make Orphan"))
				m_EditorCtx.ActiveScene->MakeOrphan(entity);

			bool correctParent = selectedEntity;
			if (selectedEntity && entity.HasComponent<ParentComponent>())
			{
				correctParent = entity.GetComponent<ParentComponent>().Parent != selectedEntity;
			}

			if (correctParent && ImGui::MenuItem("Add to children"))
			{
				Entity newParent = entity;
				Entity newChild = selectedEntity;
				m_EditorCtx.ActiveScene->MakeRelationship(newParent, newChild);
			}

			ImGui::EndPopup();
		}

		if (opened && hasChildren)
		{
			const std::vector<Entity>& children = entity.GetComponent<ChildComponent>().Children;
			for (auto entity : children)
			{
				DrawEntityNode(entity);
			}
		}

		if (opened && hasChildren)
			ImGui::TreePop();

		if (entityDeleted)
		{
			m_EditorCtx.ActiveScene->DestroyEntity(entity);
			if (selectedEntity == entity)
				m_EditorCtx.SelectedEntity = {};
		}
	}

	void SceneHierarchyPanel::DrawMaterialsEditor()
	{
		Entity selectedEntity = m_EditorCtx.SelectedEntity;
		std::vector<String> materials;
		if(selectedEntity.HasComponent<StaticMeshComponent>())
		{
			auto mesh = selectedEntity.GetComponent<StaticMeshComponent>().Mesh;
			const auto& subMeshes = mesh->GetAllSubMeshes();
			materials.reserve(subMeshes.size());
			for (uint32 i = 0; i < subMeshes.size(); ++i)
			{
				const String& material = subMeshes[i].Material->GetName();
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
				UI::PropertyCombo("Material List", materials.data(), materials.size(), &m_ActiveMaterial);

				UI::EndPropertyTable();
			}
			
			Ref<Material> material = Renderer::GetMaterialTable()->Get(m_ActiveMaterial);

			if (UI::TreeNode("Material") && UI::BeginPropertyTable())
			{
				DrawMaterialProperty(material, "u_AlbedoMap", "u_UseAlbedoMap", "u_Albedo");
				DrawMaterialProperty(material, "u_NormalMap", "u_UseNormalMap", "");
				DrawMaterialProperty(material, "u_RoughnessMap", "u_UseRoughnessMap", "u_Roughness");
				DrawMaterialProperty(material, "u_MetalnessMap", "u_UseMetalnessMap", "u_Metalness");

				UI::EndPropertyTable();
				UI::TreePop();
				ImGui::Spacing();
			}
		}
	}

	void SceneHierarchyPanel::DrawMaterialProperty(Ref<Material> mat, const String& texName, const String& useTexName, const String& uniformName)
	{
		ImGui::PushID(texName.data());
		{
			float imageSize = 45.f;
			Ref<Texture2D> texture = mat->Get<Ref<Texture2D>>(texName);

			if (UI::PropertyImage(texName.data(), texture ? texture : Renderer::GetWhiteTexture(), { imageSize, imageSize, }))
			{
				FilePath path = FileDialogs::OpenFile("Texture (*png)\0*.png\0");
				if (!path.empty())
				{
					texture = Texture2D::Create(path, texName == "u_Albedo" ? true : false);
					mat->Set(texName, texture);
					mat->Set(useTexName, (uint32)true);
				}
			}
			ImGui::SameLine();
			                                                                          
			bool useTexture = mat->Get<uint32>(useTexName);
			ImGui::Checkbox("Use", &useTexture);
			mat->Set(useTexName, (uint32)useTexture);
			
			if (!uniformName.empty())
			{
				ImGui::SameLine();
				if (uniformName == "u_Albedo")
				{
					Vector4 albedo = mat->Get<Vector4>(uniformName);
					ImGui::ColorEdit4(uniformName.data(), albedo.Data(), ImGuiColorEditFlags_NoInputs);
					mat->Set(uniformName, albedo);

					float emission = mat->Get<float>("u_Emission");
					ImGui::DragFloat("u_Emission", &emission);
					mat->Set("u_Emission", emission);
				}
				else
				{
					float uniform = mat->Get<float>(uniformName);
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::SliderFloat(uniformName.data(), &uniform, 0.f, 1.f);
					mat->Set(uniformName, uniform);
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
			UI::PushFont(UI::Fonts::Bold);
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
			DrawAddComponentEntry<SkyLightComponent>(entity, "SkyLight");

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

				UI::PropertyCombo("ScriptName", modules.data(), modules.size(), &script.Name);

				if (!ScriptEngine::ExistsScript(script.Name))
				{
					UI::EndPropertyTable();
					return;
				}

				const ScriptFieldsDescription& fieldsDesc = ScriptEngine::GetFieldsDescription(script.Name);
				ScriptFieldMap& fieldMap = ScriptEngine::GetScriptFieldMap(entity);

				for (const auto& [name, field] : fieldsDesc)
				{
					auto& fieldStorage = fieldMap.at(name);
					if (field.Type == ScriptFieldType::Int)
					{
						int data = fieldStorage.GetValue<int>();
						if (UI::PropertyDrag(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
					else if (field.Type == ScriptFieldType::Float)
					{
						float data = fieldStorage.GetValue<float>();
						if (UI::PropertyDrag(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
					else if (field.Type == ScriptFieldType::Bool)
					{
						bool data = fieldStorage.GetValue<bool>();
						if (UI::PropertyCheckbox(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
					else if (field.Type == ScriptFieldType::Vector2)
					{
						Vector2 data = fieldStorage.GetValue<Vector2>();
						if (UI::PropertyDrag(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
					else if (field.Type == ScriptFieldType::Vector3)
					{
						Vector3 data = fieldStorage.GetValue<Vector3>();
						if (UI::PropertyDrag(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
					else if (field.Type == ScriptFieldType::Vector4)
					{
						Vector4 data = fieldStorage.GetValue<Vector4>();
						if (UI::PropertyDrag(name.c_str(), &data))
							fieldStorage.SetValue(data);
					}
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

				if(UI::PropertyCombo("Projection", projTypesStrings, std::size(projTypesStrings), &currentTypeString))
				{
					camera.SetProjectionType(strToType(currentTypeString));
				}

				if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
				{
					auto perspectiveDesc = camera.GetPerspectiveData();
					bool used = false;

					float degreesFOV = Math::Degrees(perspectiveDesc.VerticalFOV);
					used = UI::PropertySlider("FOV", &degreesFOV, 0.f, 360.f) || used;
					used = UI::PropertySlider("NearClip", &perspectiveDesc.NearClip, 0.f, 10.f) || used;
					used = UI::PropertySlider("FarClip", &perspectiveDesc.FarClip, 1000.f, 30000.f) || used;

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

					used = UI::PropertyDrag("Size", &orthoDesc.Size, 0.1f) || used;
					used = UI::PropertySlider("NearClip", &orthoDesc.NearClip, -10.f, 0.f) || used;
					used = UI::PropertySlider("FarClip", &orthoDesc.FarClip, 0.f, 10.f) || used;

					if (used)
					{
						camera.SetOrthographicData(orthoDesc);
					}
				}

				UI::PropertyCheckbox("Primary", &cameraComponent.Primary);
				UI::PropertyCheckbox("FixedAspectRatio", &cameraComponent.FixedAspectRatio);

				UI::EndPropertyTable();
			}
		});

		DrawComponent<SpriteComponent>(entity, "Sprite", [](SpriteComponent& sprite)
		{
			if (UI::BeginPropertyTable())
			{
				UI::PropertyColor4("Color", sprite.Color.Data());
				UI::PropertySlider("Tiling", &sprite.TilingFactor, 1.f, 20.f);

				{
					float imageSize = 45.f;
					UI::PropertyImage("Texture", sprite.Texture.GetNativeTexture(), { imageSize, imageSize });

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							std::string_view path = (const char*)payload->Data;
							std::string_view extent = path.substr(path.size() - 4, path.size());
							if (extent == ".png\0")
							{
								sprite.Texture = Texture2D::Create(FilePath(path), true);
								sprite.Color = LinearColor::White;
							}
							else
							{
								ATN_ERROR_TAG("SceneHierarchyPanel", "Invalid texture sprite format '{}'", extent);
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
							sprite.Texture = Texture2D::Create(path, true);
							sprite.Color = LinearColor::White;
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
				UI::PropertyColor4("Color", circle.Color.Data());
				UI::PropertySlider("Thickness", &circle.Thickness, 0.f, 1.f);
				UI::PropertySlider("Fade", &circle.Fade, 0.f, 1.f);

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

				if (UI::PropertyCombo("BodyType", bodyTypesStrings, std::size(bodyTypesStrings), &currentBodyType))
				{
					rb2d.Type = strToType(currentBodyType);
				}

				UI::PropertyCheckbox("Fixed Rotation", &rb2d.FixedRotation);
					
				UI::EndPropertyTable();
			}
		});


		DrawComponent<BoxCollider2DComponent>(entity, "BoxCollider2D", [](BoxCollider2DComponent& bc2d)
		{
			if (UI::BeginPropertyTable())
			{
				UI::PropertyDrag("Offset", bc2d.Offset.Data(), 0.1f);
				UI::PropertyDrag("Size", bc2d.Size.Data(), 0.1f);
				UI::PropertySlider("Density", &bc2d.Density, 0.f, 1.f);
				UI::PropertySlider("Friction", &bc2d.Friction, 0.f, 1.f);
				UI::PropertySlider("Restitution", &bc2d.Restitution, 0.f, 1.f);
				UI::PropertySlider("RestitutionThreshold", &bc2d.RestitutionThreshold, 0.f, 1.f);

				UI::EndPropertyTable();
			}
		});

		DrawComponent<CircleCollider2DComponent>(entity, "CircleCollider2D", [](CircleCollider2DComponent& cc2d)
		{
			if (UI::BeginPropertyTable())
			{
				UI::PropertyDrag("Offset", &cc2d.Offset, 0.1f);
				UI::PropertyDrag("Radius", &cc2d.Radius, 0.1f);
				UI::PropertySlider("Density", &cc2d.Density, 0.f, 1.f);
				UI::PropertySlider("Friction", &cc2d.Friction, 0.f, 1.f);
				UI::PropertySlider("Restitution", &cc2d.Restitution, 0.f, 1.f);
				UI::PropertySlider("RestitutionThreshold", &cc2d.RestitutionThreshold, 0.f, 1.f);

				UI::EndPropertyTable();
			}
		});

		DrawComponent<StaticMeshComponent>(entity, "StaticMesh", [this, entity](StaticMeshComponent& meshComponent)
		{
			if (UI::BeginPropertyTable())
			{
				String meshFilename = meshComponent.Mesh->GetFilePath().filename().string();

				UI::PropertyText("Mesh", meshFilename.c_str());
				UI::PropertyCheckbox("Visible", &meshComponent.Visible);

				UI::EndPropertyTable();

				Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
				if (animator)
				{
					if (UI::TreeNode("Animations", true, true) && UI::BeginPropertyTable())
					{
						Ref<Animation> active = animator->GetCurrentAnimation();
						active = active ? active : (animator->GetAllAnimations().size() > 0 ? animator->GetAllAnimations()[0] : nullptr);

						std::string_view selectedAnim = active ? active->GetName().c_str() : "";

						const auto& animations = animator->GetAllAnimations();
						std::vector<std::string_view> animNames(animations.size());
						for (uint32 i = 0; i < animations.size(); ++i)
							animNames[i] = animations[i]->GetName();

						if (UI::PropertyCombo("Animation List", animNames.data(), animNames.size(), &selectedAnim))
						{
							uint32 index = std::distance(animNames.begin(), std::find(animNames.begin(), animNames.end(), selectedAnim));
							Ref<Animation> anim = animations[index];
							animator->PlayAnimation(anim);
						}

						{ 
							bool playNow = active == animator->GetCurrentAnimation();
							bool check = playNow;
							UI::PropertyCheckbox("Play", &check);

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
				UI::PropertyColor3("Color", lightComponent.Color.Data());
				UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);

				UI::EndPropertyTable();
			}
		});

		DrawComponent<PointLightComponent>(entity, "PointLight", [](PointLightComponent& lightComponent)
		{
			if (UI::BeginPropertyTable())
			{
				UI::PropertyColor3("Color", lightComponent.Color.Data());
				UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);
				UI::PropertyDrag("Radius", &lightComponent.Radius, 2.5f, 0.f, 10000.f);
				UI::PropertyDrag("FallOff", &lightComponent.FallOff, 0.1f, 0.f, 100.f);

				UI::EndPropertyTable();
			}
		});

		DrawComponent<SkyLightComponent>(entity, "SkyLight", [](SkyLightComponent& lightComponent)
		{
			static auto typeToStr = [](EnvironmentMapType type) -> std::string_view
				{
					switch (type)
					{
					case EnvironmentMapType::STATIC: return "Static";
					case EnvironmentMapType::PREETHAM: return "Preetham";
					}

					return "Invalid";
				};

			static auto strToType = [](std::string_view str) -> EnvironmentMapType
				{
					if (str == "Static")
						return EnvironmentMapType::STATIC;

					if (str == "Preetham")
						return EnvironmentMapType::PREETHAM;

					return (EnvironmentMapType)0;
				};


			if (UI::BeginPropertyTable())
			{
				UI::PropertySlider("Intensity", &lightComponent.Intensity, 0.f, 10.f);
				UI::PropertySlider("Environment Map LOD", &lightComponent.LOD, 0, ShaderDef::MAX_SKYBOX_MAP_LOD - 1);

				const auto& envMap = lightComponent.EnvironmentMap;

				const std::string_view resolutions[] = { "256", "512", "1024", "2048", "4096" };
				String selectedStr = std::to_string(envMap->GetResolution());
				std::string_view selected = selectedStr.data();

				if (UI::PropertyCombo("Resolution", resolutions, std::size(resolutions), &selected))
				{
					uint32 resolution = std::atoi(selected.data());
					envMap->SetResolution(resolution);
				}

				std::string_view typesStrings[] = { "Static", "Preetham"};
				std::string_view type = typeToStr(envMap->GetType());

				if (UI::PropertyCombo("Type", typesStrings, std::size(typesStrings), &type))
				{
					envMap->SetType(strToType(type));
				}

				if (envMap->GetType() == EnvironmentMapType::STATIC)
				{
					UI::PropertyRow("FilePath", ImGui::GetFrameHeight());
					{
						String label;

						const FilePath& envPath = envMap->GetFilePath();
						label = envPath.empty() ? "Load Environment Map" : envPath.stem().string();

						if (ImGui::Button(label.data()))
						{
							FilePath filepath = FileDialogs::OpenFile("EnvironmentMap (*hdr)\0*.hdr\0");
							if (!filepath.empty())
							{
								envMap->SetFilePath(filepath);
							}
						}
					}
				}
				else if (envMap->GetType() == EnvironmentMapType::PREETHAM)
				{
					float turbidity = envMap->GetTurbidity();
					UI::PropertySlider("Turbidity", &turbidity, 1.8f, 10.f);

					float azimuth = envMap->GetAzimuth();
					UI::PropertySlider("Azimuth", &azimuth, 0, 2 * Math::PI<float>());

					float inclination = envMap->GetInclination();
					UI::PropertySlider("Inclination", &inclination, 0, 2 * Math::PI<float>());

					envMap->SetPreethamParams(turbidity, azimuth, inclination);
				}

				UI::EndPropertyTable();
			}
		});
	}
}
