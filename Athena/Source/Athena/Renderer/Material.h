#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/ConstantBuffer.h"


namespace Athena
{
	struct MaterialDescription
	{
		String Name;

		Vector3 Albedo = Vector3(0.7);
		float Roughness = 0;
		float Metalness = 0;

		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalMap = nullptr;
		Ref<Texture2D> RoughnessMap = nullptr;
		Ref<Texture2D> MetalnessMap = nullptr;

		bool UseAlbedoTexture = false;
		bool UseNormalMap = false;
		bool UseRoughnessMap = false;
		bool UseMetalnessMap = false;
	};

	class ATHENA_API Material
	{
	public:
		struct ShaderData
		{
			Vector3 Albedo;
			float Roughness;
			float Metalness;

			bool UseAlbedoTexture;
			bool UseNormalMap;
			bool UseRoughnessMap;
			bool UseMetalnessMap;
		};

	public:
		Material();
		static Ref<Material> Create(const MaterialDescription& desc);

		const ShaderData& Bind();
		MaterialDescription& GetDescription() { return m_Description; };

	private:
		ShaderData m_ShaderData;
		MaterialDescription m_Description;
	};
}
