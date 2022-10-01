#include "SceneHierarchyPanel.h"
#include "Athena/Scene/Components.h"
#include "UI/Controllers.h"
#include "Athena/Core/PlatformUtils.h"

#include <ImGui/imgui.h>


namespace Athena
{
	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& context)
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
		ImGui::Begin("Scene Hierarchy");

		m_Context->m_Registry.each([=](auto entityID)
			{
				Entity entity(entityID, m_Context.get());

				DrawEntityNode(entity);
			});

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


		ImGui::Begin("Properties");

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
		bool opened = ImGui::TreeNodeEx((void*)(uint64)(uint32)entity, flags, tag.data());

		if (ImGui::IsItemClicked())
			m_SelectionContext = entity;

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
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
			m_Context->DestroyEntity(entity);
		}
	}

	void SceneHierarchyPanel::DrawAllComponents(Entity entity)
	{
		DrawComponent<TagComponent>(entity, "Tag", false, [](TagComponent& tagComponent)
			{
				String& tag = tagComponent.Tag;

				UI::TextInput(tag, tag);
			});
		
		DrawComponent<TransformComponent>(entity, "Transform", false, [](TransformComponent& transform)
			{
				UI::DrawVec3Controller("Position", transform.Translation, 0.0f);
				Vector3 degrees = Math::Degrees(transform.Rotation);
				UI::DrawVec3Controller("Rotation", degrees, 0.0f);
				transform.Rotation = Math::Radians(degrees);
				UI::DrawVec3Controller("Scale", transform.Scale, 1.0f);

				ImGui::Spacing();
			});

		DrawComponent<SpriteComponent>(entity, "Sprite", true, [](SpriteComponent& sprite)
			{
				UI::DrawController("Color", 0, [&sprite]() { return ImGui::ColorEdit4("##Color", sprite.Color.Data()); });
				UI::DrawController("Tiling", 0, [&sprite]() { return ImGui::DragFloat("##Tiling", &sprite.TilingFactor, 0.05f); });
				ImGui::Text("Texture");
				ImGui::SameLine();
				ImGui::ImageButton((void*)(uint64)sprite.Texture.GetNativeTexture()->GetRendererID(), { 50.f, 50.f }, { 0, 1 }, { 1, 0 });

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
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
				{
					sprite.Texture = Texture2D::WhiteTexture();
				}
			});

		DrawComponent<CircleComponent>(entity, "Circle", true, [](CircleComponent& circle)
			{
				UI::DrawController("Color       ", 0, [&circle]() { return ImGui::ColorEdit4("##Color", circle.Color.Data()); });
				UI::DrawController("Thickness", 0, [&circle]() { return ImGui::DragFloat("##Thickness", &circle.Thickness, 0.01f, 0.f, 1.f); });
				UI::DrawController("Fade         ", 0, [&circle]() { return ImGui::DragFloat("##Fade", &circle.Fade, 0.00025f, 0.f, 1.f); });
			});

		DrawComponent<CameraComponent>(entity, "Camera", true, [](CameraComponent& cameraComponent)
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

				if (UI::DrawController("Projection", 0, [&currentProjectionType]() 
					{ return ImGui::BeginCombo("##Projection", currentProjectionType.data()); }))
				{
					for (int i = 0; i < 2; ++i)
					{
						const std::string_view typeStr = typeToStr((SceneCamera::ProjectionType)i);
						bool isSelected = currentProjectionType == typeStr;
						if (ImGui::Selectable(typeStr.data(), isSelected))
						{
							currentProjectionType = typeStr;
							camera.SetProjectionType((SceneCamera::ProjectionType)i);
						}

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
					used = UI::DrawController("FOV           ", 0, [&degreesFOV]() 
						{ return ImGui::DragFloat("##FOV", &degreesFOV, 0.1f); }) || used;
					used = UI::DrawController("NearClip   ", 0, [&perspectiveDesc]()
						{ return ImGui::DragFloat("##NearClip", &perspectiveDesc.NearClip, 0.1f); }) || used;
					used = UI::DrawController("FarClip      ", 0, [&perspectiveDesc]()
						{ return ImGui::DragFloat("##FarClip", &perspectiveDesc.FarClip, 10.f); }) || used;

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

					used = UI::DrawController("Size            ", 0, [&orthoDesc]()
						{ return ImGui::DragFloat("##Size", &orthoDesc.Size, 0.1f); }) || used;
					used = UI::DrawController("NearClip   ", 0, [&orthoDesc]()
						{ return ImGui::DragFloat("##NearClip", &orthoDesc.NearClip, 0.1f); }) || used;
					used = UI::DrawController("FarClip      ", 0, [&orthoDesc]()
						{ return ImGui::DragFloat("##FarClip", &orthoDesc.FarClip, 0.1f); }) || used;

					if (used)
						camera.SetOrthographicData(orthoDesc);
				}

				UI::DrawController("Primary", 0, [&cameraComponent]()
					{ return ImGui::Checkbox("##Primary", &cameraComponent.Primary); });
				UI::DrawController("FixedAspectRatio", 0, [&cameraComponent]()
					{ return ImGui::Checkbox("##FixedAspectRatio", &cameraComponent.FixedAspectRatio); });
			});


		DrawComponent<Rigidbody2DComponent>(entity, "Rigidbody2D", true, [](Rigidbody2DComponent& rb2d)
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

				if (UI::DrawController("BodyType        ", 0, [&currentBodyType]()
					{ return ImGui::BeginCombo("##BodyType", currentBodyType.data()); }))
				{
					for (int i = 0; i < 3; ++i)
					{
						const std::string_view typeStr = typeToStr((Rigidbody2DComponent::BodyType)i);
						bool isSelected = currentBodyType == typeStr;
						if (ImGui::Selectable(typeStr.data(), isSelected))
						{
							currentBodyType = typeStr;
							rb2d.Type = (Rigidbody2DComponent::BodyType)i;
						}

						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}

				UI::DrawController("Fixed Rotation", 0, [&rb2d] { return ImGui::Checkbox("##FixedRotation", &rb2d.FixedRotation); });
			});


		DrawComponent<BoxCollider2DComponent>(entity, "BoxCollider2D", true, [](BoxCollider2DComponent& bc2d)
			{
				UI::DrawController("Offset          ", 0, [&bc2d]() { return ImGui::DragFloat2("##Offset", bc2d.Offset.Data(), 0.1f); });
				UI::DrawController("Size              ", 0, [&bc2d]() { return ImGui::DragFloat2("##Size", bc2d.Size.Data(), 0.1f); });

				UI::DrawController("Density       ", 0, [&bc2d]() { return ImGui::DragFloat("##Density", &bc2d.Density, 0.01f, 0.f, 1.f); });
				UI::DrawController("Friction       ", 0, [&bc2d]() { return ImGui::DragFloat("##Friction", &bc2d.Friction, 0.01f, 0.f, 1.f); });
				UI::DrawController("Restitution ", 0, [&bc2d]() { return ImGui::DragFloat("##Restitution", &bc2d.Restitution, 0.01f, 0.f, 1.f); });
				UI::DrawController("RestitutionThreshold", 0, [&bc2d]() { return ImGui::DragFloat("##RestitutionThreshold", &bc2d.RestitutionThreshold, 0.01f, 0.f); });
			});

		DrawComponent<CircleCollider2DComponent>(entity, "CircleCollider2D", true, [](CircleCollider2DComponent& cc2d)
			{
				UI::DrawController("Offset          ", 0, [&cc2d]() { return ImGui::DragFloat2("##Offset", cc2d.Offset.Data(), 0.1f); });
				UI::DrawController("Radius         ", 0, [&cc2d]() { return ImGui::DragFloat("##Radius", &cc2d.Radius, 0.1f); });

				UI::DrawController("Density       ", 0, [&cc2d]() { return ImGui::DragFloat("##Density", &cc2d.Density, 0.01f, 0.f, 1.f); });
				UI::DrawController("Friction       ", 0, [&cc2d]() { return ImGui::DragFloat("##Friction", &cc2d.Friction, 0.01f, 0.f, 1.f); });
				UI::DrawController("Restitution ", 0, [&cc2d]() { return ImGui::DragFloat("##Restitution", &cc2d.Restitution, 0.01f, 0.f, 1.f); });
				UI::DrawController("RestitutionThreshold", 0, [&cc2d]() { return ImGui::DragFloat("##RestitutionThreshold", &cc2d.RestitutionThreshold, 0.01f, 0.f); });
			});


		ImGui::Separator();

		if (ImGui::Button("Add"))
			ImGui::OpenPopup("Add");

		if (ImGui::BeginPopup("Add"))
		{
			if (!entity.HasComponent<CameraComponent>() && ImGui::MenuItem("Camera"))
			{
				m_SelectionContext.AddComponent<CameraComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<SpriteComponent>() && ImGui::MenuItem("Sprite"))
			{
				m_SelectionContext.AddComponent<SpriteComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<CircleComponent>() && ImGui::MenuItem("Circle"))
			{
				m_SelectionContext.AddComponent<CircleComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<Rigidbody2DComponent>() && ImGui::MenuItem("Rigidbody2D"))
			{
				m_SelectionContext.AddComponent<Rigidbody2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<BoxCollider2DComponent>() && ImGui::MenuItem("BoxCollider2D"))
			{
				m_SelectionContext.AddComponent<BoxCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			if (!entity.HasComponent<CircleCollider2DComponent>() && ImGui::MenuItem("CircleCollider2D"))
			{
				m_SelectionContext.AddComponent<CircleCollider2DComponent>();
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
}
