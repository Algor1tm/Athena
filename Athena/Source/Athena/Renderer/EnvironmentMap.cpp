#include "EnvironmentMap.h"
#include "Athena/Renderer/ComputePass.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Core/FileSystem.h"


namespace Athena
{
	Ref<EnvironmentMap> EnvironmentMap::Create(uint32 resolution)
	{
		Ref<EnvironmentMap> result = Ref<EnvironmentMap>::Create(resolution);

		return result;
	}

	EnvironmentMap::EnvironmentMap(uint32 resolution)
	{
		m_Dirty = true;
		m_Type = EnvironmentMapType::PREETHAM;
		m_Resolution = resolution;

		TextureCubeCreateInfo cubemapInfo;
		cubemapInfo.Name = "EnvironmentMap";
		cubemapInfo.Format = ImageFormat::R11G11B10F;
		cubemapInfo.Usage = ImageUsage(ImageUsage::STORAGE | ImageUsage::SAMPLED);
		cubemapInfo.InitialData = nullptr;
		cubemapInfo.Width = m_Resolution;
		cubemapInfo.Height = m_Resolution;
		cubemapInfo.MipLevels = ShaderDef::MAX_SKYBOX_MAP_LOD;
		cubemapInfo.SamplerInfo.MinFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MagFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.MipMapFilter = TextureFilter::LINEAR;
		cubemapInfo.SamplerInfo.Wrap = TextureWrap::CLAMP_TO_EDGE;

		m_EnvironmentTexture = TextureCube::Create(cubemapInfo);

		cubemapInfo.Name = "EnvIrradianceMap";
		cubemapInfo.Width = m_IrradianceMapResolution;
		cubemapInfo.Height = m_IrradianceMapResolution;
		cubemapInfo.MipLevels = 1;

		m_IrradianceTexture = TextureCube::Create(cubemapInfo);

		// Panorama To Cubemap
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "PanoramaToCubePass";
			passInfo.Shader = Renderer::GetShaderPack()->Get("PanoramaToCubemap");
			passInfo.DebugColor = { 0.2f, 0.4f, 0.6f };

			m_PanoramaToCubePass = ComputePass::Create(passInfo);
			m_PanoramaToCubePass->SetOutput(m_EnvironmentTexture);
			m_PanoramaToCubePass->SetInput("u_PanoramaTex", Renderer::GetWhiteTexture());
			m_PanoramaToCubePass->SetInput("u_Cubemap", m_EnvironmentTexture);
			m_PanoramaToCubePass->Bake();
		}

		// Preetham
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "PreethamPass";
			passInfo.Shader = Renderer::GetShaderPack()->Get("PreethamSky");
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_PreethamPass = ComputePass::Create(passInfo);
			m_PreethamPass->SetOutput(m_EnvironmentTexture);
			m_PreethamPass->SetInput("u_EnvironmentMap", m_EnvironmentTexture);
			m_PreethamPass->Bake();

			m_PreethamMaterial = Material::Create(passInfo.Shader, passInfo.Name);
		}

		// Irradiance Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "IrradiancePass";
			passInfo.Shader = Renderer::GetShaderPack()->Get("IrradianceMapConvolution");
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_IrradiancePass = ComputePass::Create(passInfo);
			m_IrradiancePass->SetOutput(m_IrradianceTexture);
			m_IrradiancePass->SetInput("u_Cubemap", m_EnvironmentTexture);
			m_IrradiancePass->SetInput("u_IrradianceMap", m_IrradianceTexture);
			m_IrradiancePass->Bake();
		}

		// Mip Filter Pipeline
		{
			ComputePassCreateInfo passInfo;
			passInfo.Name = "MipFilterPass";
			passInfo.Shader = Renderer::GetShaderPack()->Get("EnvironmentMipFilter");
			passInfo.DebugColor = { 0.6f, 0.4f, 0.2f, 1.f };

			m_MipFilterPass = ComputePass::Create(passInfo);
			m_MipFilterPass->SetOutput(m_EnvironmentTexture);
			m_MipFilterPass->SetInput("u_EnvironmentMap", m_EnvironmentTexture);
			m_MipFilterPass->Bake();

			for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				Ref<Material> mipMaterial = Material::Create(passInfo.Shader, std::format("{}_{}", passInfo.Name, mip - 1));
				mipMaterial->Set("u_EnvironmentMipImage", m_EnvironmentTexture, 0, mip);
				mipMaterial->Set("u_MipLevel", mip);

				m_MipFilterMaterials[mip] = mipMaterial;
			}
		}
	}

	void EnvironmentMap::SetType(EnvironmentMapType type)
	{
		m_Type = type;
		m_Dirty = true;
	}

	void EnvironmentMap::SetResolution(uint32 resolution)
	{
		if (m_Resolution == resolution)
			return;

		m_Resolution = resolution;
		m_EnvironmentTexture->Resize(resolution, resolution);
		m_Dirty = true;
	}

	void EnvironmentMap::SetFilePath(const FilePath& path)
	{
		if (m_FilePath == path)
			return;

		m_FilePath = path;
		m_Dirty = true;
	}

	void EnvironmentMap::SetPreethamParams(float turbidity, float azimuth, float inclination)
	{
		if (m_Turbidity == turbidity && m_Azimuth == azimuth && m_Inclination == inclination)
			return;

		m_Turbidity = turbidity;
		m_Azimuth = azimuth;
		m_Inclination = inclination;
		m_Dirty = true;
	}

	Ref<TextureCube> EnvironmentMap::GetEnvironmentTexture()
	{
		if (IsEmpty())
			return Renderer::GetBlackTextureCube();

		if (m_Dirty)
			Load();

		return m_EnvironmentTexture;
	}

	Ref<TextureCube> EnvironmentMap::GetIrradianceTexture()
	{
		if (IsEmpty())
			return Renderer::GetBlackTextureCube();

		if (m_Dirty)
			Load();

		return m_IrradianceTexture;
	}

	void EnvironmentMap::LoadFromFile(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		Ref<Texture2D> panorama = Texture2D::Create(m_FilePath);
		m_PanoramaToCubePass->SetInput("u_PanoramaTex", panorama);

		m_PanoramaToCubePass->Begin(commandBuffer);
		Renderer::Dispatch(commandBuffer, m_PanoramaToCubePass, { m_Resolution, m_Resolution, 6 });
		m_PanoramaToCubePass->End(commandBuffer);
	}

	void EnvironmentMap::LoadPreetham(const Ref<RenderCommandBuffer>& commandBuffer)
	{
		m_PreethamPass->Begin(commandBuffer);
		{
			m_PreethamMaterial->Set("u_Turbidity", m_Turbidity);
			m_PreethamMaterial->Set("u_Azimuth", m_Azimuth);
			m_PreethamMaterial->Set("u_Inclination", m_Inclination);
			m_PreethamMaterial->Bind(commandBuffer);

			Renderer::Dispatch(commandBuffer, m_PreethamPass, { m_Resolution, m_Resolution, 6 }, m_PreethamMaterial);
		}
		m_PreethamPass->End(commandBuffer);
	}

	void EnvironmentMap::Load()
	{
		Ref<RenderCommandBuffer> commandBuffer = Renderer::GetRenderCommandBuffer();

		if (m_Type == EnvironmentMapType::STATIC)
			LoadFromFile(commandBuffer);
		else if (m_Type == EnvironmentMapType::PREETHAM)
			LoadPreetham(commandBuffer);

		Renderer::BeginDebugRegion(commandBuffer, "EnvironmentBlitMipMap", { 0.6f, 0.4f, 0.2f, 1.f });
		m_EnvironmentTexture->BlitMipMap(commandBuffer, ShaderDef::MAX_SKYBOX_MAP_LOD);
		Renderer::EndDebugRegion(commandBuffer);


		m_IrradiancePass->Begin(commandBuffer);
		Renderer::Dispatch(commandBuffer, m_IrradiancePass, { m_IrradianceMapResolution, m_IrradianceMapResolution, 6 });
		m_IrradiancePass->End(commandBuffer);


		m_MipFilterPass->Begin(commandBuffer);
		for (uint32 mip = 1; mip < ShaderDef::MAX_SKYBOX_MAP_LOD; ++mip)
		{
			uint32 mipResolution = m_Resolution * Math::Pow(0.5f, (float)mip);

			m_MipFilterMaterials[mip]->Bind(commandBuffer);
			Renderer::Dispatch(commandBuffer, m_MipFilterPass, { mipResolution, mipResolution, 6 }, m_MipFilterMaterials[mip]);
		}
		m_MipFilterPass->End(commandBuffer);

		m_Dirty = false;
	}

	bool EnvironmentMap::IsEmpty()
	{
		if (m_Type == EnvironmentMapType::STATIC && !FileSystem::Exists(m_FilePath))
			return true;

		return false;
	}
}
