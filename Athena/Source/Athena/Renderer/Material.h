#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Texture.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/ConstantBuffer.h"


namespace Athena
{
	struct MaterialDescription
	{
		String Name;

		Vector3 Albedo = Vector3(1);
		float Roughness = 0;
		float Metalness = 0;
		float AmbientOcclusion = 0;

		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalMap = nullptr;
		Ref<Texture2D> RoughnessMap = nullptr;
		Ref<Texture2D> MetalnessMap = nullptr;
		Ref<Texture2D> AmbientOcclusionMap = nullptr;

		bool UseAlbedoTexture = false;
		bool UseNormalMap = false;
		bool UseRoughnessMap = false;
		bool UseMetalnessMap = false;
		bool UseAmbientOcclusionMap = false;
	};

	class ATHENA_API Material
	{
	public:
		struct ShaderData
		{
			LinearColor Albedo;
			float Roughness;
			float Metalness;
			float AmbientOcclusion;

			int UseAlbedoTexture;
			int UseNormalMap;
			int UseRoughnessMap;
			int UseMetalnessMap;
			int UseAmbientOcclusionMap;
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
