#include "atnpch.h"
#include "SceneSerializer.h"
#include "Components.h"
#include "Entity.h"

#if defined(_MSC_VER)
	#pragma warning (push, 0)
#endif

#include <yaml-cpp/yaml.h>

#if defined(_MSC_VER)
	#pragma warning (pop)
#endif

#include <fstream>
#include <sstream>


namespace YAML
{
	template <>
	struct convert<Athena::Vector2>
	{
		static Node encode(const Athena::Vector2& vec2)
		{
			Node node;
			node.push_back(vec2.x);
			node.push_back(vec2.y);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Vector2& vec2)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			vec2.x = node[0].as<float>();
			vec2.y = node[1].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::Vector3>
	{
		static Node encode(const Athena::Vector3& vec3)
		{
			Node node;
			node.push_back(vec3.x);
			node.push_back(vec3.y);
			node.push_back(vec3.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::Vector3& vec3)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			vec3.x = node[0].as<float>();
			vec3.y = node[1].as<float>();
			vec3.z = node[2].as<float>();
			return true;
		}
	};

	template <>
	struct convert<Athena::LinearColor>
	{
		static Node encode(const Athena::LinearColor& color)
		{
			Node node;
			node.push_back(color.r);
			node.push_back(color.g);
			node.push_back(color.b);
			node.push_back(color.a);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, Athena::LinearColor& color)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			color.r = node[0].as<float>();
			color.g = node[1].as<float>();
			color.b = node[2].as<float>();
			color.a = node[3].as<float>();
			return true;
		}
	};

	Emitter& operator<<(Emitter& out, const Athena::Vector2& vec2)
	{
		out << Flow;
		out << BeginSeq << vec2.x << vec2.y << EndSeq;

		return out;
	}

	Emitter& operator<<(Emitter& out, const Athena::Vector3& vec3)
	{
		out << Flow;
		out << BeginSeq << vec3.x << vec3.y << vec3.z << EndSeq;

		return out;
	}

	Emitter& operator<<(Emitter& out, const Athena::LinearColor& color)
	{
		out << Flow;
		out << BeginSeq << color.r << color.g << color.b << color.a << EndSeq;

		return out;
	}
}


namespace Athena
{
	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{

	}

	void SceneSerializer::SerializeToFile(const String& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << "Scene Name";
		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		m_Scene->m_Registry.each([&](auto entityID)
			{
				Entity entity = { entityID, m_Scene.get() };
				if (!entity)
					return;

				SerializeEntity(out, entity);
			});

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	void SceneSerializer::SerializeRuntime(const String& filepath)
	{
		ATN_CORE_ASSERT(false, "No Implemented");
	}

	bool SceneSerializer::DeserializeFromFile(const String& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		String sceneName = data["Scene"].as<String>();
		ATN_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		auto& entities = data["Entities"];
		if (entities)
		{
			for (auto entityNode : entities)
			{
				uint64 uuid = entityNode["Entity"].as<uint64>();

				String name;
				auto& tagComponentNode = entityNode["TagComponent"];
				if (tagComponentNode)
					name = tagComponentNode["Tag"].as<String>();

				Entity deserializedEntity = m_Scene->CreateEntity(name);

				auto& transformComponentNode = entityNode["TransformComponent"];
				if (transformComponentNode)
				{
					auto& transform = deserializedEntity.GetComponent<TransformComponent>();
					transform.Translation = transformComponentNode["Translation"].as<Vector3>();
					transform.Rotation = transformComponentNode["Rotation"].as<Vector3>();
					transform.Scale = transformComponentNode["Scale"].as<Vector3>();
				}

				auto& cameraComponentNode = entityNode["CameraComponent"];
				if (cameraComponentNode)
				{
					auto& cc = deserializedEntity.AddComponent<CameraComponent>();
					auto& cameraPropsNode = cameraComponentNode["Camera"];

					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraPropsNode["ProjectionType"].as<int>());

					SceneCamera::PerspectiveDESC perspectiveDesc;
					perspectiveDesc.VerticalFOV = cameraPropsNode["PerspectiveFOV"].as<float>();
					perspectiveDesc.NearClip = cameraPropsNode["PerspectiveNearClip"].as<float>();
					perspectiveDesc.FarClip = cameraPropsNode["PerspectiveFarClip"].as<float>();
					cc.Camera.SetPerspectiveData(perspectiveDesc);

					SceneCamera::OrthographicDESC orthoDesc;
					orthoDesc.Size = cameraPropsNode["OrthographicSize"].as<float>();
					orthoDesc.NearClip = cameraPropsNode["OrthographicNearClip"].as<float>();
					orthoDesc.FarClip = cameraPropsNode["OrthographicFarClip"].as<float>();
					cc.Camera.SetOrthographicData(orthoDesc);

					cc.Primary = cameraComponentNode["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponentNode["FixedAspectRatio"].as<bool>();
				}

				auto& spriteComponentNode = entityNode["SpriteComponent"];
				if (spriteComponentNode)
				{
					auto& sprite = deserializedEntity.AddComponent<SpriteComponent>();

					auto& textureNode = spriteComponentNode["Texture"];
					if (textureNode)
					{
						Ref<Texture2D> texture = Texture2D::Create(textureNode.as<String>());

						std::array<Vector2, 4> texCoords;
						auto& texCoordsNode = spriteComponentNode["TexCoords"];
						texCoords[0] = texCoordsNode["0"].as<Vector2>();
						texCoords[1] = texCoordsNode["1"].as<Vector2>();
						texCoords[2] = texCoordsNode["2"].as<Vector2>();
						texCoords[3] = texCoordsNode["3"].as<Vector2>();

						sprite.Texture = Texture2DInstance(texture, texCoords);
						sprite.TilingFactor = spriteComponentNode["TilingFactor"].as<float>();
						sprite.Color = spriteComponentNode["Tint"].as<LinearColor>();
					}
					else
					{
						sprite.Color = spriteComponentNode["Color"].as<LinearColor>();
						sprite.TilingFactor = 1.f;
					}
				}

				ATN_CORE_TRACE("Loaded Entity 'Name = {0}', 'ID = {1}'", name, uuid);
			}
		}

		return true;
	}

	bool SceneSerializer::DeserializeRuntime(const String& filepath)
	{
		ATN_CORE_ASSERT(false, "No Implemented");
		return false;
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
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << 12345678910;

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

		SerializeComponent<CameraComponent>(out, "CameraComponent", entity,
			[](YAML::Emitter& output, const CameraComponent& cameraComponent)
			{
				auto& camera = cameraComponent.Camera;

				auto& perspectiveData = camera.GetPerspectiveData();
				auto& orthoData = camera.GetOrthographicData();

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
				if (sprite.Texture.GetNativeTexture() != nullptr)
				{
					output << YAML::Key << "Texture" << YAML::Value << sprite.Texture.GetNativeTexture()->GetFilepath();

					auto& texCoords = sprite.Texture.GetTexCoords();
					output << YAML::Key << "TexCoords" << YAML::Value;
					output << YAML::BeginMap;
					output << YAML::Key << "0" << YAML::Value << texCoords[0];
					output << YAML::Key << "1" << YAML::Value << texCoords[1];
					output << YAML::Key << "2" << YAML::Value << texCoords[2];
					output << YAML::Key << "3" << YAML::Value << texCoords[3];
					output << YAML::EndMap;

					output << YAML::Key << "Tint" << YAML::Value << sprite.Color;
					output << YAML::Key << "TilingFactor" << YAML::Value << sprite.TilingFactor;
				}
				else
				{
					output << YAML::Key << "Color" << YAML::Value << sprite.Color;
				}
			});

		out << YAML::EndMap; // Entity

		ATN_CORE_TRACE("Saved Entity");
	}
}
