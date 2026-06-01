#include "Athena/Scene/SceneSerializer.h"

#include "Athena/Core/FileSystem.h"
#include "Athena/Core/YAMLTypes.h"
#include "Athena/Scene/Components.h"
#include "Athena/Scene/Entity.h"

#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>


#define WRITE_SCRIPT_FIELD(nativeType, fieldType)   \
		case ScriptFieldType::fieldType:		    \
			output << field.GetValue<nativeType>(); \
			break								  
	
#define READ_SCRIPT_FIELD(nativeType, fieldType, defaultValue)									   \
		case ScriptFieldType::fieldType:														   \
		{																						   \
			nativeType data = TryReadYAMLValue<nativeType>(scriptFieldNode, "Data", defaultValue); \
			field.SetValue(data);																   \
			break;																				   \
		}

namespace Athena
{
	SceneSerializer::SceneSerializer(WeakRef<Scene> scene)
		: m_Scene(scene)
	{

	}

	bool SceneSerializer::SerializeToFile(const FilePath& path)
	{
		ATN_PROFILE_FUNC();

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << m_Scene->GetSceneName();
		out << YAML::Key << "Handle" << YAML::Value << m_Scene->Handle;
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;

		// Need to iterate in revers order to keep entities order when deserializing
		const auto& view = m_Scene->GetAllEntitiesWith<IDComponent>();
		for (auto it = view.rbegin(); it != view.rend(); ++it)
		{
			Entity entity = { *it, m_Scene.Raw() };
			if (!entity)
				return false;

			SerializeEntity(out, entity);
		}

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(path);
		fout << out.c_str();

		return true;
	}

	bool SceneSerializer::DeserializeFromFile(const FilePath& path)
	{
		ATN_PROFILE_FUNC();

		if (!FileSystem::Exists(path))
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Invalid scene filepath {}", path);
			return false;
		}

		YAML::Node data = YAML::TryLoadYAMLFile(path);

		if (!data["Scene"])
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to deserialize scene {0}", path);
			return false;
		}

		String sceneName = TryReadYAMLValue<String>(data, "Scene", "Unnamed");
		m_Scene->SetSceneName(sceneName);

		AssetHandle handle = TryReadYAMLValue<AssetHandle>(data, "Handle", 0);
		m_Scene->Handle = handle;

		const auto& entities = data["Entities"];
		if (!entities)
			return false;

		for (const auto& entityNode : entities)
		{
			UUID uuid = 0;
			{
				const auto& uuidComponentNode = entityNode["IDComponent"];
				if (uuidComponentNode)
					uuid = TryReadYAMLValue<UUID>(uuidComponentNode, "ID", 0);
			}

			String name;
			{
				const auto& tagComponentNode = entityNode["TagComponent"];
				if (tagComponentNode)
					name = TryReadYAMLValue<String>(tagComponentNode, "Tag", "Unnamed");
			}

			Entity deserializedEntity = m_Scene->CreateEntity(name, uuid);

			{
				const auto& transformComponentNode = entityNode["TransformComponent"];
				if (transformComponentNode)
				{
					auto& transform = deserializedEntity.GetComponent<TransformComponent>();
					transform.Translation = TryReadYAMLValue<Vector3>(transformComponentNode, "Translation", Vector3(0, 0, 0));
					transform.Rotation = TryReadYAMLValue<Quaternion>(transformComponentNode, "Rotation", Quaternion::Identity());
					transform.Scale = TryReadYAMLValue<Vector3>(transformComponentNode, "Scale", Vector3(1, 1, 1));
				}
			}

			{
				const auto& scriptComponentNode = entityNode["ScriptComponent"];
				if (scriptComponentNode)
				{
					auto& script = deserializedEntity.AddComponent<ScriptComponent>();
					script.Name = TryReadYAMLValue<String>(scriptComponentNode, "ScriptName", "Invalid");

					const YAML::Node& scriptFieldsNode = scriptComponentNode["ScriptFields"];
					if (scriptFieldsNode && ScriptEngine::IsScriptExists(script.Name))
					{
						ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(deserializedEntity);

						for (const YAML::Node& scriptFieldNode : scriptFieldsNode)
						{
							String name = TryReadYAMLValue<String>(scriptFieldNode, "Name", "Invalid");
							std::string typeString = TryReadYAMLValue<String>(scriptFieldNode, "Type", "Invalid");
							ScriptFieldType type = Utils::ScriptFieldTypeFromString(typeString);

							if (!fieldMap || !fieldMap->contains(name))
							{
								ATN_CORE_WARN_TAG("AssetManager", "Uknown script field name '{}' (field will be discarded, script name - {}, entity name - {})!", 
									name, script.Name, deserializedEntity.GetName());
								continue;
							}

							ScriptFieldStorage& field = fieldMap->at(name);

							if (type != field.GetType())
							{
								ATN_CORE_WARN_TAG("AssetManager", "Script field type '{}' does not match with original type '{}' (field will be discarded, script name - {}, entity name - {})!", 
									Utils::ScriptFieldTypeToString(type), Utils::ScriptFieldTypeToString(field.GetType()), script.Name, deserializedEntity.GetName());
								continue;
							}

							switch (type)
							{
							READ_SCRIPT_FIELD(bool,   Bool,   false);
							READ_SCRIPT_FIELD(char,   Char,   '0');
							READ_SCRIPT_FIELD(byte,   Byte,   0);
							READ_SCRIPT_FIELD(int16,  Int16,  0);
							READ_SCRIPT_FIELD(int32,  Int32,  0);
							READ_SCRIPT_FIELD(int64,  Int64,  0);
							READ_SCRIPT_FIELD(uint16, UInt16, 0);
							READ_SCRIPT_FIELD(uint32, UInt32, 0);
							READ_SCRIPT_FIELD(uint64, UInt64, 0);
							READ_SCRIPT_FIELD(float,  Float,  0.f);
							READ_SCRIPT_FIELD(double, Double, 0.0);
							}
						}
					}
					else if (!ScriptEngine::IsScriptExists(script.Name))
					{
						ATN_CORE_WARN_TAG("AssetManager", "Uknown script '{}' (script and all fields will be discarded)", script.Name);
					}
				}
			}

			{
				const auto& cameraComponentNode = entityNode["CameraComponent"];
				if (cameraComponentNode)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();
					const auto& cameraPropsNode = cameraComponentNode["Camera"];

					if (cameraPropsNode)
					{
						int projectionType = TryReadYAMLValue<int>(cameraPropsNode, "ProjectionType", (int)SceneCamera::ProjectionType::Perspective);
						cc.Camera.SetProjectionType((SceneCamera::ProjectionType)projectionType);

						SceneCamera::PerspectiveData perspectiveData;
						perspectiveData.VerticalFOV = TryReadYAMLValue<float>(cameraPropsNode, "PerspectiveFOV", Math::Radians(45.f));
						perspectiveData.NearClip = TryReadYAMLValue<float>(cameraPropsNode, "PerspectiveNearClip", 0.1f);
						perspectiveData.FarClip = TryReadYAMLValue<float>(cameraPropsNode, "PerspectiveFarClip", 1000.f);
						cc.Camera.SetPerspectiveData(perspectiveData);

						SceneCamera::OrthographicData orthoData;
						orthoData.Size = TryReadYAMLValue<float>(cameraPropsNode, "OrthographicSize", 10.f);
						orthoData.NearClip = TryReadYAMLValue<float>(cameraPropsNode, "OrthographicNearClip", -1.f);
						orthoData.FarClip = TryReadYAMLValue<float>(cameraPropsNode, "OrthographicFarClip", 1.f);
						cc.Camera.SetOrthographicData(orthoData);
					}

					cc.Primary = TryReadYAMLValue<bool>(cameraComponentNode, "Primary", true);
					cc.FixedAspectRatio = TryReadYAMLValue<bool>(cameraComponentNode, "FixedAspectRatio", false);
				}
			}

			{
				const auto& spriteComponentNode = entityNode["SpriteComponent"];
				if (spriteComponentNode)
				{
					auto& sprite = deserializedEntity.AddComponent<SpriteComponent>();

					sprite.Space = (Renderer2DSpace)TryReadYAMLValue<int>(spriteComponentNode, "Space", (int)Renderer2DSpace::WorldSpace);
					sprite.Color = TryReadYAMLValue<LinearColor>(spriteComponentNode, "Color", LinearColor::Black);
					sprite.TextureHandle = TryReadYAMLValue<AssetHandle>(spriteComponentNode, "TextureHandle", 0);
					sprite.TilingFactor = TryReadYAMLValue<float>(spriteComponentNode, "TilingFactor", 1.f);
				}
			}

			{
				const auto& circleComponentNode = entityNode["CircleComponent"];
				if (circleComponentNode)
				{
					auto& circle = deserializedEntity.AddComponent<CircleComponent>();

					circle.Space = (Renderer2DSpace)TryReadYAMLValue<int>(circleComponentNode, "Space", (int)Renderer2DSpace::WorldSpace);
					circle.Color = TryReadYAMLValue<LinearColor>(circleComponentNode, "Color", LinearColor::Black);
					circle.Thickness = TryReadYAMLValue<float>(circleComponentNode, "Thickness", 1.f);
					circle.Fade = TryReadYAMLValue<float>(circleComponentNode, "Fade", 0.005f);
				}
			}

			{
				const auto& textComponentNode = entityNode["TextComponent"];
				if (textComponentNode)
				{
					auto& text = deserializedEntity.AddComponent<TextComponent>();

					text.Text = TryReadYAMLValue<String>(textComponentNode, "Text", "");
					text.FontHandle = TryReadYAMLValue<AssetHandle>(textComponentNode, "FontHandle", 0);
					text.Space = (Renderer2DSpace)TryReadYAMLValue<int>(textComponentNode, "Space", (int)Renderer2DSpace::WorldSpace);
					text.Color = TryReadYAMLValue<LinearColor>(textComponentNode, "Color", LinearColor::Black);
					text.MaxWidth = TryReadYAMLValue<float>(textComponentNode, "MaxWidth", 10.f);
					text.Kerning = TryReadYAMLValue<float>(textComponentNode, "Kerning", 0.f);
					text.LineSpacing = TryReadYAMLValue<float>(textComponentNode, "LineSpacing", 0.f);
					text.Shadowing = TryReadYAMLValue<bool>(textComponentNode, "Shadowing", false);
					text.ShadowDistance = TryReadYAMLValue<float>(textComponentNode, "ShadowDistance", 1.f);
					text.ShadowColor = TryReadYAMLValue<LinearColor>(textComponentNode, "ShadowColor", LinearColor::Black);
				}
			}

			{
				const auto& rigidbody2DComponentNode = entityNode["Rigidbody2DComponent"];
				if (rigidbody2DComponentNode)
				{
					auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();

					rb2d.Type = (Rigidbody2DComponent::BodyType)TryReadYAMLValue<int>(rigidbody2DComponentNode, "BodyType", (int)Rigidbody2DComponent::BodyType::STATIC);
					rb2d.FixedRotation = TryReadYAMLValue<bool>(rigidbody2DComponentNode, "FixedRotation", false);
				}
			}

			{
				const auto& boxCollider2DComponentNode = entityNode["BoxCollider2DComponent"];
				if (boxCollider2DComponentNode)
				{
					auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();

					bc2d.Offset = TryReadYAMLValue<Vector2>(boxCollider2DComponentNode, "Offset", { 0.f, 0.f });
					bc2d.Size = TryReadYAMLValue<Vector2>(boxCollider2DComponentNode, "Size", { 0.5f, 0.5f });
					bc2d.Density = TryReadYAMLValue<float>(boxCollider2DComponentNode, "Density", 1.f);
					bc2d.Friction = TryReadYAMLValue<float>(boxCollider2DComponentNode, "Friction", 0.5f);
					bc2d.Restitution = TryReadYAMLValue<float>(boxCollider2DComponentNode, "Restitution", 0.f);
					bc2d.RestitutionThreshold = TryReadYAMLValue<float>(boxCollider2DComponentNode, "RestitutionThreshold", 0.5f);
				}
			}

			{
				const auto& circleCollider2DComponentNode = entityNode["CircleCollider2DComponent"];
				if (circleCollider2DComponentNode)
				{
					auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();

					cc2d.Offset = TryReadYAMLValue<Vector2>(circleCollider2DComponentNode, "Offset", { 0.f, 0.f });
					cc2d.Radius = TryReadYAMLValue<float>(circleCollider2DComponentNode, "Radius", 0.5f);
					cc2d.Density = TryReadYAMLValue<float>(circleCollider2DComponentNode, "Density", 1.f);
					cc2d.Friction = TryReadYAMLValue<float>(circleCollider2DComponentNode, "Friction", 0.5f);
					cc2d.Restitution = TryReadYAMLValue<float>(circleCollider2DComponentNode, "Restitution", 0.f);
					cc2d.RestitutionThreshold = TryReadYAMLValue<float>(circleCollider2DComponentNode, "RestitutionThreshold", 0.5f);
				}
			}

			{
				const auto& meshComponentNode = entityNode["MeshComponent"];
				if (meshComponentNode)
				{
					auto& meshComp = deserializedEntity.AddComponent<MeshComponent>();

					meshComp.MeshHandle = TryReadYAMLValue<AssetHandle>(meshComponentNode, "MeshHandle", 0);
					meshComp.MeshNodeIndex = TryReadYAMLValue<uint32>(meshComponentNode, "MeshNodeIndex", 0);
					meshComp.Visible = TryReadYAMLValue<bool>(meshComponentNode, "Visible", true);

					meshComp.ResetMaterials();
				}
			}

			{
				const auto& controllerNode = entityNode["AnimationControllerComponent"];
				if (controllerNode)
				{
					auto& controllerComp = deserializedEntity.AddComponent<AnimationControllerComponent>();

					AssetHandle meshHandle = TryReadYAMLValue<AssetHandle>(controllerNode, "MeshHandle", 0);
					controllerComp.AnimationController = AnimationController::Create(meshHandle);
				}
			}

			{
				const auto directionalLightComponent = entityNode["DirectionalLightComponent"];
				if (directionalLightComponent)
				{
					auto& lightComp = deserializedEntity.AddComponent<DirectionalLightComponent>();

					lightComp.Color = TryReadYAMLValue<LinearColor>(directionalLightComponent, "Color", LinearColor::Black);
					lightComp.Intensity = TryReadYAMLValue<float>(directionalLightComponent, "Intensity", 1.f);
					lightComp.CastShadows = TryReadYAMLValue<bool>(directionalLightComponent, "CastShadows", false);
					lightComp.LightSize = TryReadYAMLValue<float>(directionalLightComponent, "LightSize", 0.4f);
				}
			}

			{
				const auto pointLightComponent = entityNode["PointLightComponent"];
				if (pointLightComponent)
				{
					auto& lightComp = deserializedEntity.AddComponent<PointLightComponent>();

					lightComp.Color = TryReadYAMLValue<LinearColor>(pointLightComponent, "Color", LinearColor::Black);
					lightComp.Intensity = TryReadYAMLValue<float>(pointLightComponent, "Intensity", 1.f);
					lightComp.Radius = TryReadYAMLValue<float>(pointLightComponent, "Radius", 10.f);
					lightComp.FallOff = TryReadYAMLValue<float>(pointLightComponent, "FallOff", 1.f);
				}
			}

			{
				const auto pointLightComponent = entityNode["SpotLightComponent"];
				if (pointLightComponent)
				{
					auto& lightComp = deserializedEntity.AddComponent<SpotLightComponent>();

					lightComp.Color = TryReadYAMLValue<LinearColor>(pointLightComponent, "Color", LinearColor::Black);
					lightComp.Intensity = TryReadYAMLValue<float>(pointLightComponent, "Intensity", 1.f);
					lightComp.SpotAngle = TryReadYAMLValue<float>(pointLightComponent, "SpotAngle", 30.f);
					lightComp.InnerFallOff = TryReadYAMLValue<float>(pointLightComponent, "InnerFallOff", 1.f);
					lightComp.Range = TryReadYAMLValue<float>(pointLightComponent, "Range", 10.f);
					lightComp.RangeFallOff = TryReadYAMLValue<float>(pointLightComponent, "RangeFallOff", 1.f);
				}
			}

			{
				const auto skyLightComponent = entityNode["SkyLightComponent"];
				if (skyLightComponent)
				{
					auto& lightComp = deserializedEntity.AddComponent<SkyLightComponent>();

					lightComp.EnvMapHandle = TryReadYAMLValue<AssetHandle>(skyLightComponent, "EnvMapHandle", 0);
					lightComp.Type = (EnvironmentMapType)TryReadYAMLValue<uint32>(skyLightComponent, "Type", (uint32)EnvironmentMapType::PREETHAM);
					lightComp.Intensity = TryReadYAMLValue<float>(skyLightComponent, "Intensity", 1.f);
					lightComp.LOD = TryReadYAMLValue<float>(skyLightComponent, "LOD", 0.f);
					lightComp.Preetham.Turbidity = TryReadYAMLValue<float>(skyLightComponent, "Turbidity", 2.f);
					lightComp.Preetham.Azimuth = TryReadYAMLValue<float>(skyLightComponent, "Azimuth", 0.f);
					lightComp.Preetham.Inclination = TryReadYAMLValue<float>(skyLightComponent, "Inclination", 0.f);
					lightComp.Preetham.Resolution = TryReadYAMLValue<uint32>(skyLightComponent, "Resolution", 128);
				}
			}
		}

		// Build Entity Hierarchy
		for (const auto& entityNode : entities)
		{
			uint64 uuid = 0;
			{
				const auto& uuidComponentNode = entityNode["IDComponent"];
				if (uuidComponentNode)
					uuid = TryReadYAMLValue<UUID>(uuidComponentNode, "ID", 0);
			}

			Entity entity = m_Scene->GetEntityByUUID(uuid);
				
			{
				const auto& parentComponentNode = entityNode["ParentComponent"];
				if (parentComponentNode)
				{
					UUID parentID = TryReadYAMLValue<UUID>(parentComponentNode, "Parent", 0);
					Entity parent = m_Scene->GetEntityByUUID(parentID);

					if (parent)
						m_Scene->MakeRelationship(parent, entity);
				}
			}
		}

		return true;
	}

	template <typename Component, typename Func>
	static void SerializeComponent(YAML::Emitter& out, const String& name, Entity entity, Func serialize)
	{
		if (entity.HasComponent<Component>())
		{
			out << YAML::Key << name;
			out << YAML::BeginMap;

			serialize(out, entity.GetComponent<Component>());

			out << YAML::EndMap;
		}
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		if (!entity.HasComponent<IDComponent>() && !entity.HasComponent<TagComponent>())
		{
			ATN_CORE_ERROR_TAG("Serializer", "Entity cannot been serialized(does not have UUIDComponent and TagComponent)");
			return;
		}

		out << YAML::BeginMap; // Entity

		SerializeComponent<IDComponent>(out, "IDComponent", entity, [](YAML::Emitter& output, const IDComponent& id) 
			{
				output << YAML::Key << "ID" << YAML::Value << id.ID;
			});

		SerializeComponent<TagComponent>(out, "TagComponent", entity, 
			[](YAML::Emitter& output, const TagComponent& tag) 
			{
				output << YAML::Key << "Tag" << YAML::Value << tag.Tag;
			});

		SerializeComponent<TransformComponent>(out, "TransformComponent", entity,
			[](YAML::Emitter& output, const TransformComponent& transform)
			{
				output << YAML::Key << "Translation" << YAML::Value << transform.Translation;
				output << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
				output << YAML::Key << "Scale" << YAML::Value << transform.Scale;
			});

		SerializeComponent<ParentComponent>(out, "ParentComponent", entity,
			[](YAML::Emitter& output, const ParentComponent& parentCmp)
			{
				UUID parentID = parentCmp.Parent.GetComponent<IDComponent>().ID;
				output << YAML::Key << "Parent" << YAML::Value << parentID;
			});

		SerializeComponent<ScriptComponent>(out, "ScriptComponent", entity,
			[entity](YAML::Emitter& output, const ScriptComponent& script)
			{
				output << YAML::Key << "ScriptName" << YAML::Value << script.Name;

				const ScriptFieldMap* fieldMap = ScriptEngine::GetScriptFieldMap(entity);
				if (!fieldMap)
					return;

				output << YAML::Key << "ScriptFields" << YAML::Value;
				output << YAML::BeginSeq;

				for (const auto& [name, field] : *fieldMap)
				{
					output << YAML::BeginMap;

					output << YAML::Key << "Name" << YAML::Value << name.data();
					output << YAML::Key << "Type" << YAML::Value << Utils::ScriptFieldTypeToString(field.GetType());
					output << YAML::Key << "Data" << YAML::Value;

					switch (field.GetType())
					{
					WRITE_SCRIPT_FIELD(bool,    Bool);
					WRITE_SCRIPT_FIELD(char,    Char);
					WRITE_SCRIPT_FIELD(byte,    Byte);
					WRITE_SCRIPT_FIELD(int16,   Int16);
					WRITE_SCRIPT_FIELD(int32,   Int32);
					WRITE_SCRIPT_FIELD(int64,   Int64);
					WRITE_SCRIPT_FIELD(uint16,  UInt16);
					WRITE_SCRIPT_FIELD(uint32,  UInt32);
					WRITE_SCRIPT_FIELD(uint64,  UInt64);
					WRITE_SCRIPT_FIELD(float,   Float);
					WRITE_SCRIPT_FIELD(double,  Double);
					WRITE_SCRIPT_FIELD(Vector2, Vector2);
					WRITE_SCRIPT_FIELD(Vector3, Vector3);
					WRITE_SCRIPT_FIELD(Vector4, Vector4);
					}

					output << YAML::EndMap;
				}

				output << YAML::EndSeq;
			});

		SerializeComponent<CameraComponent>(out, "CameraComponent", entity,
			[](YAML::Emitter& output, const CameraComponent& cameraComponent)
			{
				const auto& camera = cameraComponent.Camera;
				
				const auto& perspectiveData = camera.GetPerspectiveData();
				const auto& orthoData = camera.GetOrthographicData();

				output << YAML::Key << "Camera" << YAML::Value;
				output << YAML::BeginMap;
				output << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
				output << YAML::Key << "PerspectiveFOV" << YAML::Value << perspectiveData.VerticalFOV;
				output << YAML::Key << "PerspectiveNearClip" << YAML::Value << perspectiveData.NearClip;
				output << YAML::Key << "PerspectiveFarClip" << YAML::Value << perspectiveData.FarClip;
				output << YAML::Key << "OrthographicSize" << YAML::Value << orthoData.Size;
				output << YAML::Key << "OrthographicNearClip" << YAML::Value << orthoData.NearClip;
				output << YAML::Key << "OrthographicFarClip" << YAML::Value << orthoData.FarClip;
				output << YAML::EndMap;

				output << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
				output << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;
			});

		SerializeComponent<SpriteComponent>(out, "SpriteComponent", entity,
			[](YAML::Emitter& output, const SpriteComponent& sprite)
			{
				output << YAML::Key << "Space" << YAML::Value << (int)sprite.Space;
				output << YAML::Key << "Color" << YAML::Value << sprite.Color;
				output << YAML::Key << "TextureHandle" << YAML::Value << sprite.TextureHandle;
				output << YAML::Key << "TilingFactor" << YAML::Value << sprite.TilingFactor;
			});

		SerializeComponent<CircleComponent>(out, "CircleComponent", entity, [](YAML::Emitter& output, const CircleComponent& circle)
			{
				output << YAML::Key << "Space" << YAML::Value << (int)circle.Space;
				output << YAML::Key << "Color" << YAML::Value << circle.Color;
				output << YAML::Key << "Thickness" << YAML::Value << circle.Thickness;
				output << YAML::Key << "Fade" << YAML::Value << circle.Fade;
			});

		SerializeComponent<TextComponent>(out, "TextComponent", entity, [](YAML::Emitter& output, const TextComponent& text)
			{
				output << YAML::Key << "Text" << YAML::Value << text.Text;
				output << YAML::Key << "FontHandle" << YAML::Value << text.FontHandle;
				output << YAML::Key << "Space" << YAML::Value << (int)text.Space;
				output << YAML::Key << "Color" << YAML::Value << text.Color;
				output << YAML::Key << "MaxWidth" << YAML::Value << text.MaxWidth;
				output << YAML::Key << "Kerning" << YAML::Value << text.Kerning;
				output << YAML::Key << "LineSpacing" << YAML::Value << text.LineSpacing;
				output << YAML::Key << "Shadowing" << YAML::Value << text.Shadowing;
				output << YAML::Key << "ShadowDistance" << YAML::Value << text.ShadowDistance;
				output << YAML::Key << "ShadowColor" << YAML::Value << text.ShadowColor;
			});

		SerializeComponent<Rigidbody2DComponent>(out, "Rigidbody2DComponent", entity,
			[](YAML::Emitter& output, const Rigidbody2DComponent& rb2d)
			{
				output << YAML::Key << "BodyType" << YAML::Value << (int)rb2d.Type;
				output << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;
			});

		SerializeComponent<BoxCollider2DComponent>(out, "BoxCollider2DComponent", entity,
			[](YAML::Emitter& output, const BoxCollider2DComponent& bc2d)
			{
				output << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
				output << YAML::Key << "Size" << YAML::Value << bc2d.Size;

				output << YAML::Key << "Density" << YAML::Value << bc2d.Density;
				output << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
				output << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
				output << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;
			});

		SerializeComponent<CircleCollider2DComponent>(out, "CircleCollider2DComponent", entity,
			[](YAML::Emitter& output, const CircleCollider2DComponent& cc2d)
			{
				output << YAML::Key << "Offset" << YAML::Value << cc2d.Offset;
				output << YAML::Key << "Radius" << YAML::Value << cc2d.Radius;

				output << YAML::Key << "Density" << YAML::Value << cc2d.Density;
				output << YAML::Key << "Friction" << YAML::Value << cc2d.Friction;
				output << YAML::Key << "Restitution" << YAML::Value << cc2d.Restitution;
				output << YAML::Key << "RestitutionThreshold" << YAML::Value << cc2d.RestitutionThreshold;
			});

		SerializeComponent<MeshComponent>(out, "MeshComponent", entity,
			[](YAML::Emitter& output, const MeshComponent& meshComponent)
			{
				output << YAML::Key << "MeshHandle" << YAML::Value << meshComponent.MeshHandle;
				output << YAML::Key << "MeshNodeIndex" << YAML::Value << meshComponent.MeshNodeIndex;
				output << YAML::Key << "Visible" << YAML::Value << meshComponent.Visible;
			});

		SerializeComponent<AnimationControllerComponent>(out, "AnimationControllerComponent", entity,
			[](YAML::Emitter& output, const AnimationControllerComponent& controllerComponent)
			{
				output << YAML::Key << "MeshHandle" << YAML::Value << controllerComponent.AnimationController->GetMeshHandle();
			});

		SerializeComponent<DirectionalLightComponent>(out, "DirectionalLightComponent", entity,
			[](YAML::Emitter& output, const DirectionalLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "CastShadows" << YAML::Value << lightComponent.CastShadows;
				output << YAML::Key << "LightSize" << YAML::Value << lightComponent.LightSize;
			});

		SerializeComponent<PointLightComponent>(out, "PointLightComponent", entity,
			[](YAML::Emitter& output, const PointLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "Radius" << YAML::Value << lightComponent.Radius;
				output << YAML::Key << "FallOff" << YAML::Value << lightComponent.FallOff;
			});

		SerializeComponent<SpotLightComponent>(out, "SpotLightComponent", entity,
			[](YAML::Emitter& output, const SpotLightComponent& lightComponent)
			{
				output << YAML::Key << "Color" << YAML::Value << lightComponent.Color;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "SpotAngle" << YAML::Value << lightComponent.SpotAngle;
				output << YAML::Key << "InnerFallOff" << YAML::Value << lightComponent.InnerFallOff;
				output << YAML::Key << "Range" << YAML::Value << lightComponent.Range;
				output << YAML::Key << "RangeFallOff" << YAML::Value << lightComponent.RangeFallOff;
			});

		SerializeComponent<SkyLightComponent>(out, "SkyLightComponent", entity,
			[](YAML::Emitter& output, const SkyLightComponent& lightComponent)
			{
				output << YAML::Key << "EnvMapHandle" << lightComponent.EnvMapHandle;
				output << YAML::Key << "Type" << (int)lightComponent.Type;
				output << YAML::Key << "Intensity" << YAML::Value << lightComponent.Intensity;
				output << YAML::Key << "LOD" << YAML::Value << lightComponent.LOD;
				output << YAML::Key << "Turbidity" << lightComponent.Preetham.Turbidity;
				output << YAML::Key << "Azimuth" << lightComponent.Preetham.Azimuth;
				output << YAML::Key << "Inclination" << lightComponent.Preetham.Inclination;
				output << YAML::Key << "Resolution" << lightComponent.Preetham.Resolution;
			});

		out << YAML::EndMap;
	}
}
