#include "SceneHierarchyPanel.h"

#include "Athena/Asset/Editor/AssetFileExtensions.h"
#include "Athena/Asset/Editor/MeshImporter.h"
#include "Athena/Core/FileDialogs.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Input/Input.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/EngineTextures.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scripting/ScriptEngine.h"
#include "Athena/UI/UI.h"
#include "Athena/UI/Theme.h"


#include "Panels/PanelManager.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/MaterialEditorPanel.h"
#include "EditorResources.h"

#include <ImGui/imgui.h>


namespace Athena
{
#define SCRIPT_FIELD_SCALAR_INPUT(nativeType, fieldType, imguiType)	\
	case ScriptFieldType::fieldType:	\
	{	\
		nativeType data = field.GetValue<nativeType>();  \
		UI::PropertyRow(name.data(), ImGui::GetFrameHeight());  \
		ImGui::PushID(name.data());  \
		if (ImGui::DragScalar("##InputScalar", imguiType, &data))  \
			field.SetValue(data);  \
		ImGui::PopID();  \
		break;  \
	}

#define SCRIPT_FIELD_SCALAR_INPUT_N(type, components)	\
	case ScriptFieldType::type:	\
	{	\
		type data = field.GetValue<type>();  \
		UI::PropertyRow(name.data(), ImGui::GetFrameHeight());  \
		ImGui::PushID(name.data());  \
		if (ImGui::DragScalarN("##InputScalar", ImGuiDataType_Float, data.Data(), components))  \
			field.SetValue(data);  \
		ImGui::PopID();  \
		break;  \
	}


	static void DrawVec3Property(std::string_view label, Vector3& values, float defaultValues)
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


	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<EditorContext>& context)
		: Panel(SCENE_HIERARCHY_PANEL_ID, context)
	{
		UI::RegisterEnum("Renderer2DSpace");
		UI::EnumAdd("Renderer2DSpace", 1, "World Space");
		UI::EnumAdd("Renderer2DSpace", 2, "Screen Space");
		UI::EnumAdd("Renderer2DSpace", 3, "Billboard");
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
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

		// When scene state changes to Play or Simulate, scene is copied, which leads
		// to reversed order of entities, to "prevent" this we iterate entities in reversed order
		if (m_EditorCtx.SceneState == SceneState::Edit)
		{
			for (auto entt : view)
			{
				Entity entity = Entity(entt, m_EditorCtx.ActiveScene.Raw());
				if (!entity.HasComponent<ParentComponent>())
				{
					DrawEntityNode(entity);
				}
			}
		}
		else
		{
			for (auto it = view.rbegin(); it != view.rend(); ++it)
			{
				Entity entity = Entity(*it, m_EditorCtx.ActiveScene.Raw());
				if (!entity.HasComponent<ParentComponent>())
				{
					DrawEntityNode(entity);
				}
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

		if (ImGui::IsMouseDown(ImGuiPopupFlags_MouseButtonLeft) && ImGui::IsWindowHovered())
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

		static char idString[20];
		sprintf(idString, "%llu", (uint64)entity.GetComponent<IDComponent>().ID);

		if (ImGui::BeginPopupContextItem(idString, ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			if (hasParent && ImGui::MenuItem("Make Orphan"))
				m_EditorCtx.ActiveScene->MakeOrphan(entity);

			if (selectedEntity && entity != selectedEntity && ImGui::MenuItem("Add to children"))
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

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		float fullWidth = ImGui::GetContentRegionAvail().x;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 3.f, 3.f });
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5.f, 5.f });
		UI::ShiftCursorX(5.f);
		if (ImGui::Button("Edit"))
		{
			if(!m_EditTagComponent)
				ImGui::SetKeyboardFocusHere();

			m_EditTagComponent = !m_EditTagComponent;
		}

		ImGui::SameLine();

		auto& tag = entity.GetComponent<TagComponent>().Tag;
		if (m_EditTagComponent)
		{
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
			UI::TextInput("TagInputText", tag);

			if (Input::IsKeyPressed(Keyboard::Enter) || Input::IsKeyPressed(Keyboard::Escape))
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
			DrawAddComponentEntry<TextComponent>(entity, "Text");
			DrawAddComponentEntry<Rigidbody2DComponent>(entity, "Rigidbody2D");
			DrawAddComponentEntry<BoxCollider2DComponent>(entity, "BoxCollider2D");
			DrawAddComponentEntry<CircleCollider2DComponent>(entity, "CircleCollider2D");
			DrawAddComponentEntry<MeshComponent>(entity, "Mesh");
			DrawAddComponentEntry<AnimationControllerComponent>(entity, "AnimationController");
			DrawAddComponentEntry<DirectionalLightComponent>(entity, "DirectionalLight");
			DrawAddComponentEntry<PointLightComponent>(entity, "PointLight");
			DrawAddComponentEntry<SpotLightComponent>(entity, "SpotLight");
			DrawAddComponentEntry<SkyLightComponent>(entity, "SkyLight");

			ImGui::EndPopup();
		}

		ImGui::PopStyleVar(2);

		ImGui::Spacing();

		DrawComponent<TransformComponent>(entity, "TRANSFORM", [](TransformComponent& transform)
		{
			DrawVec3Property("Translation", transform.Translation, 0.0f);

			Vector3 degrees = Math::Degrees(transform.Rotation.AsEulerAngles());
			DrawVec3Property("Rotation", degrees, 0.0f);
			transform.Rotation = Math::Radians(degrees);

			DrawVec3Property("Scale", transform.Scale, 1.0f);

			return true;
		});

		DrawComponent<ScriptComponent>(entity, "SCRIPT", [entity](ScriptComponent& script)
		{
 			const std::vector<String>& scripts = ScriptEngine::GetAvailableScripts();
			UI::PropertyCombo("Script Name", scripts.data(), scripts.size(), &script.Name);

			ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(entity);

			if (fieldMap == nullptr)
				return true;

			for (auto& [name, field] : *fieldMap)
			{
				switch (field.GetType())
				{
				SCRIPT_FIELD_SCALAR_INPUT(byte,   Byte,   ImGuiDataType_U8);
				SCRIPT_FIELD_SCALAR_INPUT(int16,  Int16,  ImGuiDataType_S16);
				SCRIPT_FIELD_SCALAR_INPUT(uint16, UInt16, ImGuiDataType_U16);
				SCRIPT_FIELD_SCALAR_INPUT(int32,  Int32,  ImGuiDataType_S32);
				SCRIPT_FIELD_SCALAR_INPUT(uint32, UInt32, ImGuiDataType_U32);
				SCRIPT_FIELD_SCALAR_INPUT(int64,  Int64,  ImGuiDataType_S64);
				SCRIPT_FIELD_SCALAR_INPUT(uint64, UInt64, ImGuiDataType_U64);
				SCRIPT_FIELD_SCALAR_INPUT(float,  Float,  ImGuiDataType_Float);
				SCRIPT_FIELD_SCALAR_INPUT(double, Double, ImGuiDataType_Double);
				
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector2, 2);
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector3, 3);
				SCRIPT_FIELD_SCALAR_INPUT_N(Vector4, 4);

				case ScriptFieldType::Bool:
				{
					bool data = field.GetValue<bool>();
					if (UI::PropertyCheckbox(name.data(), &data))
						field.SetValue(data);
					break;
				}
				case ScriptFieldType::Char:
				{
					char data = field.GetValue<char>();  
					UI::PropertyRow(name.data(), ImGui::GetFrameHeight());

					static char buffer[2];
					buffer[0] = data;
					buffer[1] = '\0';

					ImGui::PushID(name.data());
					if (ImGui::InputText("##InputText", buffer, sizeof(buffer)))
						field.SetValue(buffer[0]);
					ImGui::PopID();  
					break;
				}
				}
			}

			return true;
		});

		DrawComponent<CameraComponent>(entity, "CAMERA", [](CameraComponent& cameraComponent)
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
				used = UI::PropertySlider("Near Clip", &perspectiveDesc.NearClip, 0.f, 10.f) || used;
				used = UI::PropertySlider("Far Clip", &perspectiveDesc.FarClip, 1000.f, 30000.f) || used;

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
				used = UI::PropertySlider("Near Clip", &orthoDesc.NearClip, -10.f, 0.f) || used;
				used = UI::PropertySlider("Far Clip", &orthoDesc.FarClip, 0.f, 10.f) || used;

				if (used)
				{
					camera.SetOrthographicData(orthoDesc);
				}
			}

			UI::PropertyCheckbox("Primary", &cameraComponent.Primary);
			UI::PropertyCheckbox("Fixed Aspect Ratio", &cameraComponent.FixedAspectRatio);

			return true;
		});

		DrawComponent<SpriteComponent>(entity, "SPRITE", [&entity](SpriteComponent& sprite)
		{
			UI::PropertyColor4("Color", sprite.Color.Data());

			bool isDefault = sprite.TextureHandle == AssetHandle(0);
			Ref<TextureAsset> textureAsset = AssetManager::GetAsset<TextureAsset>(sprite.TextureHandle);
			bool isValid = textureAsset != nullptr && !isDefault;

			Ref<Texture2D> texture;
			if (isDefault)
				texture = EngineTextures::GetWhiteTexture();
			else if (!isValid)
				texture = EditorResources::GetIcon("EmptyTexture");
			else
				texture = textureAsset->GetRenderTexture();

			float imageSize = 45.f * ImGui::GetIO().FontGlobalScale;
			UI::PropertyImage("Texture", texture, { imageSize, imageSize });

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

					if (cbPayload->AssetType == AssetType::Texture)
					{
						sprite.TextureHandle = cbPayload->AssetHandle;
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			ImVec2 cursor = ImGui::GetCursorPos();
			if (ImGui::Button("Browse"))
			{
				std::vector<String> textureExts = AssetFileExtensions::GetAssetExtensionsList(AssetType::Texture);
				FilePath path = FileDialogs::OpenFile("Select Texture", "Texture files", textureExts, Project::GetAssetDirectory());
				AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(path);

				if (Project::GetEditorAssetManager()->IsAssetHandleValid(handle))
				{
					sprite.TextureHandle = handle;
				}
			}

			if (!isDefault)
			{
				ImGui::SetCursorPos({ cursor.x, cursor.y + imageSize / 1.8f });
				if (ImGui::Button("Reset"))
					sprite.TextureHandle = AssetHandle(0);
			}


			if(UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&sprite.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertySlider("Tiling", &sprite.TilingFactor, 1.f, 20.f);

			return true;
		});

		DrawComponent<CircleComponent>(entity, "CIRCLE", [&entity](CircleComponent& circle)
		{
			UI::PropertyColor4("Color", circle.Color.Data());

			if (UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&circle.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertySlider("Thickness", &circle.Thickness, 0.f, 1.f);
			UI::PropertySlider("Fade", &circle.Fade, 0.f, 1.f);

			return true;
		});

		DrawComponent<TextComponent>(entity, "TEXT", [&entity](TextComponent& text)
		{
			UI::PropertyRow("Text", ImGui::GetFrameHeight());
			UI::InputTextMultiline("##TextInput", text.Text, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 6), ImGuiInputTextFlags_AllowTabInput);

			bool isDefault = text.FontHandle == AssetHandle(0);
			const FilePath& assetPath = Project::GetEditorAssetManager()->GetAssetFilePath(text.FontHandle);
			bool isHandleValid = isDefault || Project::GetEditorAssetManager()->IsAssetHandleValid(text.FontHandle);

			String fontName = isDefault ? "Default" : assetPath.filename().string();

			UI::PropertyRow("Font", ImGui::GetFrameHeight() + 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 4 });

			if (!isHandleValid)
			{
				fontName = "<Invalid>";
				ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);
			}

			if (ImGui::Button(fontName.c_str()))
			{
				std::vector<String> fontExts = AssetFileExtensions::GetAssetExtensionsList(AssetType::Font);
				FilePath filepath = FileDialogs::OpenFile("Select Font", "Font files", fontExts, Project::GetAssetDirectory());
				AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(filepath);

				if (Project::GetEditorAssetManager()->IsAssetHandleValid(handle))
				{
					text.FontHandle = handle;
				}
			}

			if (!isHandleValid)
				ImGui::PopStyleColor();

			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

					if (cbPayload->AssetType == AssetType::Font)
					{
						text.FontHandle = cbPayload->AssetHandle;
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (!isDefault)
			{
				ImGui::SameLine();

				auto& style = ImGui::GetStyle();
				ImVec2 labelSize = ImGui::CalcTextSize("  ");
				ImVec2 size = ImGui::CalcItemSize({ 0, 0 }, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 0.5f);
				UI::ShiftCursorY(style.FramePadding.y);
				if (ImGui::InvisibleButton("ResetFont", size))
				{
					text.FontHandle = AssetHandle(0);
				}

				UI::ButtonImage((EditorResources::GetIcon("ContentBrowser_Refresh")));
			}

			if (UI::PropertyEnumCombo("Space", ATN_STRINGIFY_MACRO(Renderer2DSpace), (void*)&text.Space))
				entity.GetComponent<TransformComponent>().Translation = Vector3(0.0);

			UI::PropertyColor4("Color", text.Color.Data());
			UI::PropertyDrag("Max Width", &text.MaxWidth, 0.2f);
			UI::PropertyDrag("Kerning", &text.Kerning, 0.025f);
			UI::PropertyDrag("Line Spacing", &text.LineSpacing, 0.025f);
			UI::PropertyCheckbox("Shadowing", &text.Shadowing);

			if (text.Shadowing)
			{
				UI::PropertyDrag("Shadow Distance", &text.ShadowDistance, 0.1f);
				UI::PropertyColor4("Shadow Color", text.ShadowColor.Data());
			}

			return true;
		});

		DrawComponent<Rigidbody2DComponent>(entity, "RIGID BODY 2D", [](Rigidbody2DComponent& rb2d)
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


			std::string_view bodyTypesStrings[] = { "Static", "Dynamic", "Kinematic"};
			std::string_view currentBodyType = typeToStr(rb2d.Type);

			if (UI::PropertyCombo("BodyType", bodyTypesStrings, std::size(bodyTypesStrings), &currentBodyType))
			{
				rb2d.Type = strToType(currentBodyType);
			}

			UI::PropertyCheckbox("Fixed Rotation", &rb2d.FixedRotation);
					
			return true;
		});


		DrawComponent<BoxCollider2DComponent>(entity, "BOX COLLIDER 2D", [](BoxCollider2DComponent& bc2d)
		{
			UI::PropertyDrag("Offset", bc2d.Offset.Data(), 0.1f);
			UI::PropertyDrag("Size", bc2d.Size.Data(), 0.1f);
			UI::PropertySlider("Density", &bc2d.Density, 0.f, 1.f);
			UI::PropertySlider("Friction", &bc2d.Friction, 0.f, 1.f);
			UI::PropertySlider("Restitution", &bc2d.Restitution, 0.f, 1.f);
			UI::PropertySlider("Restitution Threshold", &bc2d.RestitutionThreshold, 0.f, 1.f);

			return true;
		});

		DrawComponent<CircleCollider2DComponent>(entity, "CIRCLE COLLIDER 2D", [](CircleCollider2DComponent& cc2d)
		{
			UI::PropertyDrag("Offset", &cc2d.Offset, 0.1f);
			UI::PropertyDrag("Radius", &cc2d.Radius, 0.1f);
			UI::PropertySlider("Density", &cc2d.Density, 0.f, 1.f);
			UI::PropertySlider("Friction", &cc2d.Friction, 0.f, 1.f);
			UI::PropertySlider("Restitution", &cc2d.Restitution, 0.f, 1.f);
			UI::PropertySlider("Restitution Threshold", &cc2d.RestitutionThreshold, 0.f, 1.f);

			return true;
		});

		DrawComponent<MeshComponent>(entity, "MESH", [this, entity](MeshComponent& meshComponent)
		{
			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(meshComponent.MeshHandle);
			bool isMeshValid = mesh != nullptr;

			String name = isMeshValid ? AssetManager::GetAssetFilePath(meshComponent.MeshHandle).stem().string() : "<Invalid>";

			UI::PropertyRow("Mesh", ImGui::GetFrameHeight() + 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 4 });
			if(!isMeshValid)
				ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);

			if (ImGui::Button(name.c_str()))
			{
				std::vector<String> meshExts = AssetFileExtensions::GetAssetExtensionsList(AssetType::Mesh);
				FilePath filepath = FileDialogs::OpenFile("Select Mesh", "Mesh files", meshExts, Project::GetAssetDirectory());
				AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(filepath);

				if (AssetManager::IsAssetHandleValid(handle))
				{
					meshComponent.MeshHandle = handle;
				}
			}

			if (!isMeshValid)
				ImGui::PopStyleColor();
			ImGui::PopStyleVar();

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

					if (cbPayload->AssetType == AssetType::Mesh)
					{
						meshComponent.MeshHandle = cbPayload->AssetHandle;
					}
				}
				ImGui::EndDragDropTarget();
			}

			UI::PropertyCheckbox("Visible", &meshComponent.Visible);
			UI::EndPropertyTable();

			if (isMeshValid && UI::TreeNode("MATERIALS", true, true) && UI::BeginPropertyTable())
			{
				Ref<MeshImportSettings> importSettings = Project::GetEditorAssetManager()->GetAssetImportSettings(meshComponent.MeshHandle).As<MeshImportSettings>();

				MaterialTable& table = mesh->GetMaterialTable();
				MaterialTable& overrideTable = importSettings->OverrideMaterials;

				for (auto& [name, materialHandle] : table)
				{
					UI::PropertyRow(name.data(), ImGui::GetFrameHeight());

					Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);
					bool isInvalid = material == nullptr;
					const char* label = isInvalid ? "<Invalid>" : material->GetMaterial()->GetName().data();

					if (isInvalid)
						ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);

					if (ImGui::ButtonEx(label, ImVec2(0, 0), ImGuiButtonFlags_PressedOnDoubleClick) && !isInvalid)
					{
						auto panel = PanelManager::GetPanel<MaterialEditorPanel>(MATERIAL_EDITOR_PANEL_ID);
						panel->SetActiveMaterial(materialHandle);
					}

					if (isInvalid)
						ImGui::PopStyleColor();

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
						{
							CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

							if (cbPayload->AssetType == AssetType::Material)
							{
								table.at(name) = cbPayload->AssetHandle;
								overrideTable[name] = cbPayload->AssetHandle;
							}
						}

						ImGui::EndDragDropTarget();
					}
				}

				UI::EndPropertyTable();
				UI::TreePop();
			}
			return false;
		});

		DrawComponent<AnimationControllerComponent>(entity, "ANIMATION CONTROLLER", [](AnimationControllerComponent& controllerComponent) 
		{
			Ref<AnimationController> controller = controllerComponent.AnimationController;

			if (!controller)
				return true;

			Ref<Mesh> mesh = AssetManager::GetAsset<Mesh>(controller->GetMeshHandle());

			if (!mesh)
				return true;

			const auto& animations = mesh->GetAnimations();

			Ref<Animation> active = controller->GetCurrentAnimation();
			active = active ? active : (animations.size() > 0 ? animations[0] : nullptr);

			std::string_view selectedAnim = active ? active->GetName().c_str() : "";

			std::vector<std::string_view> animNames(animations.size());
			for (uint32 i = 0; i < animations.size(); ++i)
				animNames[i] = animations[i]->GetName();

			if (UI::PropertyCombo("Animation List", animNames.data(), animNames.size(), &selectedAnim))
			{
				uint32 index = std::distance(animNames.begin(), std::find(animNames.begin(), animNames.end(), selectedAnim));
				Ref<Animation> anim = animations[index];
				controller->PlayAnimation(anim);
			}

			{
				bool playNow = active == controller->GetCurrentAnimation();
				bool check = playNow;
				UI::PropertyCheckbox("Play", &check);

				if (check && !playNow)
					controller->PlayAnimation(active);
				else if (!check && playNow)
					controller->ClearAnimation();

				if (check)
				{
					Ref<Animation> anim = controller->GetCurrentAnimation();
					uint32 ticks = anim->GetTicksPerSecond();
					float animTime = controller->GetAnimationTime() / (float)ticks;
					ImGui::SameLine();
					ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
					ImGui::SliderFloat("##Duration", &animTime, 0, (anim->GetDuration() - 1) / (float)ticks, nullptr, ImGuiSliderFlags_NoInput);
					controller->SetAnimationTime(animTime * (float)ticks);
				}
			}

			return true;
		});

		DrawComponent<DirectionalLightComponent>(entity, "DIRECTIONAL LIGHT", [](DirectionalLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 100.f);
			UI::PropertyCheckbox("Cast Shadows", &lightComponent.CastShadows);
			UI::PropertyDrag("Light Size", &lightComponent.LightSize, 0.025f, 0.f, 100.f);

			return true;
		});

		DrawComponent<PointLightComponent>(entity, "POINT LIGHT", [](PointLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);
			UI::PropertyDrag("Radius", &lightComponent.Radius, 2.5f, 0.f, 10000.f);
			UI::PropertyDrag("Fall Off", &lightComponent.FallOff, 0.1f, 0.f, 100.f);

			return true;
		});

		DrawComponent<SpotLightComponent>(entity, "SPOT LIGHT", [](SpotLightComponent& lightComponent)
		{
			UI::PropertyColor3("Color", lightComponent.Color.Data());
			UI::PropertyDrag("Intensity", &lightComponent.Intensity, 0.1f, 0.f, 10000.f);
			UI::PropertySlider("Spot Angle", &lightComponent.SpotAngle, 0.f, 180.f);
			UI::PropertyDrag("Inner Fall Off", &lightComponent.InnerFallOff, 0.1f, 0.f, 100.f);
			UI::PropertyDrag("Range", &lightComponent.Range, 2.5f, 0.f, 10000.f);
			UI::PropertyDrag("Range Fall Off", &lightComponent.RangeFallOff, 0.1f, 0.f, 100.f);

			return true;
		});

		DrawComponent<SkyLightComponent>(entity, "SKY LIGHT", [](SkyLightComponent& lightComponent)
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


			UI::PropertySlider("Intensity", &lightComponent.Intensity, 0.f, 10.f);
			UI::PropertySlider("LOD", &lightComponent.LOD, 0, ShaderDef::MAX_SKYBOX_MAP_LOD - 1);

			EnvironmentMapType type = lightComponent.Type;

			std::string_view typesStrings[] = { "Static", "Preetham"};
			std::string_view typeStr = typeToStr(type);

			if (UI::PropertyCombo("Type", typesStrings, std::size(typesStrings), &typeStr))
			{
				lightComponent.Type = strToType(typeStr);
			}
			if (type == EnvironmentMapType::STATIC)
			{
				bool isHandleValid = Project::GetEditorAssetManager()->IsAssetHandleValid(lightComponent.EnvMapHandle);
				const FilePath& envPath = Project::GetEditorAssetManager()->GetAssetFilePath(lightComponent.EnvMapHandle);
				String label = envPath.stem().string();

				UI::PropertyRow("EnvironmentMap", ImGui::GetFrameHeight());

				if (!isHandleValid)
				{
					label = "<Invalid>";
					ImGui::PushStyleColor(ImGuiCol_Text, UI::GetTheme().ErrorText);
				}

				if (ImGui::Button(label.data()))
				{
					std::vector<String> envMapExts = AssetFileExtensions::GetAssetExtensionsList(AssetType::EnvironmentMap);
					FilePath filepath = FileDialogs::OpenFile("Select Environment map", "EnvMap files", envMapExts, Project::GetAssetDirectory());
					AssetHandle handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(filepath);

					if (Project::GetEditorAssetManager()->IsAssetHandleValid(handle))
						lightComponent.EnvMapHandle = handle;
				}

				if (!isHandleValid)
					ImGui::PopStyleColor();

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
					{
						CBDragDropPayload* cbPayload = (CBDragDropPayload*)payload->Data;

						if (cbPayload->AssetType == AssetType::EnvironmentMap)
							lightComponent.EnvMapHandle = cbPayload->AssetHandle;
					}
					ImGui::EndDragDropTarget();
				}

			}
			else if (type == EnvironmentMapType::PREETHAM)
			{


				UI::PropertySlider("Resolution", &lightComponent.Preetham.Resolution, 1.8f, 10.f);
				UI::PropertySlider("Turbidity", &lightComponent.Preetham.Turbidity, 1.8f, 10.f);
				UI::PropertySlider("Azimuth", &lightComponent.Preetham.Azimuth, 0, 2 * Math::PI<float>());
				UI::PropertySlider("Inclination", &lightComponent.Preetham.Inclination, 0, 2 * Math::PI<float>());
			}

			return true;
		});
	}
}
