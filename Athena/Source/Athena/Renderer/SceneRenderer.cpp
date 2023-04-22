#include "SceneRenderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/SceneRenderer2D.h"
#include "Athena/Renderer/RenderQueue.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	struct CameraData
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		Vector4 CameraPosition;
		float NearClip;
		float FarClip;
	};

	struct SceneData
	{
		float Exposure = 1.f;
		float Gamma = 2.2f;
	};

	struct EnvironmentMapData
	{
		float EnvironmentMapLOD = 0.f;
		float Intensity = 1.f;
	};

	struct EntityData
	{
		Matrix4 TransformMatrix;
		int32 EntityID = -1;
		bool IsAnimated = false;
	};

	struct LightData
	{
		DirectionalLight DirectionalLightBuffer[ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLightBuffer[ShaderConstants::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;
	};

	struct ShadowsData
	{
		struct CascadeSplitInfo
		{
			Vector2 LightFrustumPlanes;
			float SplitDepth;
			float __Padding;
		};

		Matrix4 LightViewProjMatrices[ShaderConstants::SHADOW_CASCADES_COUNT];
		Matrix4 LightViewMatrices[ShaderConstants::SHADOW_CASCADES_COUNT];
		CascadeSplitInfo CascadeSplits[ShaderConstants::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float LightSize = 0.5f;
		bool SoftShadows = true;
	};

	struct SceneRendererData
	{
		Ref<Framebuffer> HDRFramebuffer;
		Ref<Framebuffer> FinalFramebuffer;
		Ref<Framebuffer> ShadowMap;
		Ref<Framebuffer> EntityIDFramebuffer;

		Ref<TextureSampler> PCF_Sampler;

		RenderQueue GeometryQueue;

		Ref<Environment> ActiveEnvironment;

		CameraData CameraDataBuffer;
		SceneData SceneDataBuffer;
		EnvironmentMapData EnvMapDataBuffer;
		EntityData EntityDataBuffer;
		LightData LightDataBuffer;
		ShadowsData ShadowsDataBuffer;

		Ref<ConstantBuffer> CameraConstantBuffer;
		Ref<ConstantBuffer> SceneConstantBuffer;
		Ref<ConstantBuffer> EnvMapConstantBuffer;
		Ref<ConstantBuffer> EntityConstantBuffer;
		Ref<ConstantBuffer> MaterialConstantBuffer;
		Ref<ConstantBuffer> ShadowsConstantBuffer;

		Ref<ShaderStorageBuffer> LightShaderStorageBuffer;
		Ref<ShaderStorageBuffer> BoneTransformsShaderStorageBuffer;

		ShadowSettings ShadowSettings;

		Antialising AntialisingMethod = Antialising::MSAA_4X;

		DebugView CurrentDebugView = DebugView::NONE;
		SceneRenderer::Statistics Stats;

		const uint32 EnvMapResolution = 2048;
		const uint32 IrradianceMapResolution = 128;

		const uint32 ShadowMapResolution = 2048;
	};

	static SceneRendererData s_Data;


	void SceneRenderer::Init()
	{
		FramebufferDescription fbDesc;
		fbDesc.Attachments = { TextureFormat::RGBA16F, TextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Layers = 1;
		fbDesc.Samples = 4;

		s_Data.HDRFramebuffer = Framebuffer::Create(fbDesc);

		fbDesc.Attachments = { TextureFormat::RGBA8, TextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Layers = 1;
		fbDesc.Samples = 4;

		s_Data.FinalFramebuffer = Framebuffer::Create(fbDesc);

		fbDesc.Attachments = { TextureFormat::DEPTH32F };
		fbDesc.Width = s_Data.ShadowMapResolution;
		fbDesc.Height = s_Data.ShadowMapResolution;
		fbDesc.Layers = ShaderConstants::SHADOW_CASCADES_COUNT;
		fbDesc.Samples = 1;

		s_Data.ShadowMap = Framebuffer::Create(fbDesc);

		fbDesc.Attachments = { TextureFormat::RED_INTEGER, TextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Layers = 1;
		fbDesc.Samples = 1;

		s_Data.EntityIDFramebuffer = Framebuffer::Create(fbDesc);

		TextureSamplerDescription samplerDesc;
		samplerDesc.MinFilter = TextureFilter::LINEAR;
		samplerDesc.MagFilter = TextureFilter::LINEAR;
		samplerDesc.Wrap = TextureWrap::CLAMP_TO_BORDER;
		samplerDesc.BorderColor = LinearColor::White;
		samplerDesc.CompareMode = TextureCompareMode::REF;
		samplerDesc.CompareFunc = TextureCompareFunc::LEQUAL;

		s_Data.PCF_Sampler = TextureSampler::Create(samplerDesc);

		s_Data.CameraConstantBuffer = ConstantBuffer::Create(sizeof(CameraData), BufferBinder::CAMERA_DATA);
		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(SceneData), BufferBinder::SCENE_DATA);
		s_Data.EnvMapConstantBuffer = ConstantBuffer::Create(sizeof(EnvironmentMapData), BufferBinder::ENVIRONMENT_MAP_DATA);
		s_Data.EntityConstantBuffer = ConstantBuffer::Create(sizeof(EntityData), BufferBinder::ENTITY_DATA);
		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), BufferBinder::MATERIAL_DATA);
		s_Data.ShadowsConstantBuffer = ConstantBuffer::Create(sizeof(ShadowsData), BufferBinder::SHADOWS_DATA);

		s_Data.LightShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(LightData), BufferBinder::LIGHT_DATA);
		s_Data.BoneTransformsShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(Matrix4) * ShaderConstants::MAX_NUM_BONES, BufferBinder::BONES_DATA);
	}

	void SceneRenderer::Shutdown()
	{
		
	}

	void SceneRenderer::OnWindowResized(uint32 width, uint32 height)
	{
		s_Data.HDRFramebuffer->Resize(width, height);
		s_Data.FinalFramebuffer->Resize(width, height);
		s_Data.EntityIDFramebuffer->Resize(width, height);
	}

	void SceneRenderer::BeginScene(const CameraInfo& cameraInfo, const Ref<Environment>& environment)
	{
		s_Data.GeometryQueue.Clear();
		s_Data.ActiveEnvironment = environment;

		s_Data.CameraDataBuffer.ViewMatrix = cameraInfo.ViewMatrix;
		s_Data.CameraDataBuffer.ProjectionMatrix = cameraInfo.ProjectionMatrix;
		s_Data.CameraDataBuffer.CameraPosition = Math::AffineInverse(cameraInfo.ViewMatrix)[3];
		s_Data.CameraDataBuffer.NearClip = cameraInfo.NearClip;
		s_Data.CameraDataBuffer.FarClip = cameraInfo.FarClip;

		s_Data.SceneDataBuffer.Exposure = s_Data.ActiveEnvironment->Exposure;
		s_Data.SceneDataBuffer.Gamma = s_Data.ActiveEnvironment->Gamma;

		s_Data.EnvMapDataBuffer.EnvironmentMapLOD = s_Data.ActiveEnvironment->EnvironmentMapLOD;
		s_Data.EnvMapDataBuffer.Intensity = s_Data.ActiveEnvironment->AmbientLightIntensity;

		ComputeCascadeSplits();
	}

	void SceneRenderer::EndScene()
	{
		ShadowMapPass();
		GeometryPass();
		SkyboxPass();
		SceneCompositePass();
		DebugViewPass();

		s_Data.FinalFramebuffer->UnBind();
		s_Data.ActiveEnvironment = nullptr;

		s_Data.LightDataBuffer.DirectionalLightCount = 0;
		s_Data.LightDataBuffer.PointLightCount = 0;
	}

	void SceneRenderer::BeginEntityIDPass()
	{
		s_Data.EntityIDFramebuffer->Bind();
		Renderer::Clear(LinearColor::White);
		s_Data.EntityIDFramebuffer->ClearAttachment(0, -1);
	}

	void SceneRenderer::EndEntityIDPass()
	{
		s_Data.EntityIDFramebuffer->UnBind();
	}

	void SceneRenderer::FlushEntityIDs()
	{
		s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));
		RenderGeometry("EntityID", false);
	}

	void SceneRenderer::BeginFrame() {}
	void SceneRenderer::EndFrame()
	{
		s_Data.FinalFramebuffer->ResolveMutlisampling();
		s_Data.FinalFramebuffer->UnBind();
	}

	void SceneRenderer::SubmitLight(const DirectionalLight& dirLight)
	{
		if (ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT == s_Data.LightDataBuffer.DirectionalLightCount)
		{
			ATN_CORE_WARN("SceneRenderer::SubmitLight: Attempt to submit more than {} DirectionalLights!", ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT);
			return;
		}

		uint32 currentIndex = s_Data.LightDataBuffer.DirectionalLightCount;
		s_Data.LightDataBuffer.DirectionalLightBuffer[currentIndex] = dirLight;
		s_Data.LightDataBuffer.DirectionalLightCount++;

		s_Data.Stats.DirectionalLightsCount++;
	}

	void SceneRenderer::SubmitLight(const PointLight& pointLight)
	{
		if (ShaderConstants::MAX_POINT_LIGHT_COUNT == s_Data.LightDataBuffer.PointLightCount)
		{
			ATN_CORE_WARN("SceneRenderer::SubmitLight: Attempt to submit more than {} PointLights!", ShaderConstants::MAX_POINT_LIGHT_COUNT);
			return;
		}

		s_Data.LightDataBuffer.PointLightBuffer[s_Data.LightDataBuffer.PointLightCount] = pointLight;
		s_Data.LightDataBuffer.PointLightCount++;

		s_Data.Stats.PointLightsCount ++;
	}

	void SceneRenderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			DrawCallInfo info;
			info.VertexBuffer = vertexBuffer;
			info.Material = material;
			info.Animator = animator;
			info.Transform = transform;
			info.EntityID = entityID;

			s_Data.GeometryQueue.Push(info);
			s_Data.Stats.GeometryCount++;
		}
		else
		{
			ATN_CORE_WARN("SceneRenderer::Submit(): Attempt to submit nullptr vertexBuffer!");
		}
	}

	void SceneRenderer::RenderGeometry(std::string_view shader, bool useMaterials)
	{
		if (s_Data.GeometryQueue.Empty())
			return;

		Renderer::BindShader(shader);

		// Render Static Meshes
		s_Data.EntityDataBuffer.IsAnimated = false;

		while (s_Data.GeometryQueue.HasStaticMeshes())
		{
			auto& info = s_Data.GeometryQueue.Next();

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (useMaterials && s_Data.GeometryQueue.UpdateMaterial())
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));

			Renderer::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
		}

		// Render Animated Meshes
		s_Data.EntityDataBuffer.IsAnimated = true;

		while (s_Data.GeometryQueue.HasAnimMeshes())
		{
			auto& info = s_Data.GeometryQueue.Next();

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (s_Data.GeometryQueue.UpdateAnimator())
			{
				const auto& boneTransforms = info.Animator->GetBoneTransforms();
				s_Data.BoneTransformsShaderStorageBuffer->SetData(boneTransforms.data(), sizeof(Matrix4) * boneTransforms.size());
			}

			if (useMaterials && s_Data.GeometryQueue.UpdateMaterial())
			{
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));
			}

			Renderer::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
		}

		s_Data.GeometryQueue.Reset();
	}

	void SceneRenderer::ShadowMapPass()
	{
		s_Data.ShadowMap->Bind();
		Renderer::Clear(LinearColor::White);
		Renderer::SetCullMode(CullFace::FRONT);

		s_Data.ShadowsDataBuffer.SoftShadows = s_Data.ShadowSettings.SoftShadows;
		s_Data.ShadowsDataBuffer.LightSize = s_Data.ShadowSettings.LightSize;
		s_Data.ShadowsDataBuffer.MaxDistance = s_Data.ShadowSettings.MaxDistance;
		s_Data.ShadowsDataBuffer.FadeOut = s_Data.ShadowSettings.FadeOut;

		if(s_Data.ShadowSettings.EnableShadows && s_Data.LightDataBuffer.DirectionalLightCount > 0) // For now only 1 directional light 
		{
			ComputeCascadeSpaceMatrices(s_Data.LightDataBuffer.DirectionalLightBuffer[0]);
			s_Data.ShadowsConstantBuffer->SetData(&s_Data.ShadowsDataBuffer, sizeof(ShadowsData));

			RenderGeometry("DirShadowMap", false);
		}
	}

	void SceneRenderer::GeometryPass()
	{
		s_Data.HDRFramebuffer->Bind();
		Renderer::Clear(LinearColor::Black);
		Renderer::SetCullMode(CullFace::BACK);

		s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));
		s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));
		s_Data.EnvMapConstantBuffer->SetData(&s_Data.EnvMapDataBuffer, sizeof(EnvironmentMapData));
		s_Data.LightShaderStorageBuffer->SetData(&s_Data.LightDataBuffer, sizeof(LightData));

		if (s_Data.ActiveEnvironment && s_Data.ActiveEnvironment->EnvironmentMap)
		{
			s_Data.ActiveEnvironment->EnvironmentMap->Bind();
			Renderer::GetBRDF_LUT()->Bind(TextureBinder::BRDF_LUT);
		}

		s_Data.ShadowMap->BindDepthAttachment(TextureBinder::SHADOW_MAP);
		s_Data.ShadowMap->BindDepthAttachment(TextureBinder::PCF_SAMPLER);
		s_Data.PCF_Sampler->Bind(TextureBinder::PCF_SAMPLER);

		s_Data.GeometryQueue.Sort();
		RenderGeometry("PBR", true);

		s_Data.PCF_Sampler->UnBind(TextureBinder::PCF_SAMPLER);
	}

	void SceneRenderer::SkyboxPass()
	{
		if (s_Data.ActiveEnvironment && s_Data.ActiveEnvironment->EnvironmentMap)
		{
			Matrix4 originalViewMatrix = s_Data.CameraDataBuffer.ViewMatrix;
			// Remove translation
			s_Data.CameraDataBuffer.ViewMatrix[3][0] = 0;
			s_Data.CameraDataBuffer.ViewMatrix[3][1] = 0;
			s_Data.CameraDataBuffer.ViewMatrix[3][2] = 0;

			s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));
			s_Data.CameraDataBuffer.ViewMatrix = originalViewMatrix;

			s_Data.EnvMapConstantBuffer->SetData(&s_Data.EnvMapDataBuffer, sizeof(EnvironmentMapData));

			s_Data.ActiveEnvironment->EnvironmentMap->Bind();

			Renderer::BindShader("Skybox");

			Renderer::DrawTriangles(Renderer::GetCubeVertexBuffer());
			s_Data.Stats.DrawCalls++;
		}
	}

	void SceneRenderer::SceneCompositePass()
	{
		s_Data.HDRFramebuffer->ResolveMutlisampling();

		s_Data.FinalFramebuffer->Bind();
		Renderer::Clear(LinearColor::Black);

		Renderer::BindShader("SceneComposite");

		s_Data.HDRFramebuffer->BindColorAttachment(0, 0);
		s_Data.HDRFramebuffer->BindDepthAttachment(1);

		Renderer::DrawTriangles(Renderer::GetQuadVertexBuffer());
	}

	void SceneRenderer::DebugViewPass()
	{
		s_Data.FinalFramebuffer->Bind();
		s_Data.CameraConstantBuffer->SetData(&s_Data.CameraDataBuffer, sizeof(CameraData));

		if (s_Data.CurrentDebugView == DebugView::WIREFRAME)
			RenderGeometry("Debug_Wireframe", false);

		else if (s_Data.CurrentDebugView == DebugView::SHADOW_CASCADES)
			RenderGeometry("Debug_ShadowCascades", false);
	}

	void SceneRenderer::ComputeCascadeSplits()
	{
		float cameraNear = s_Data.CameraDataBuffer.NearClip;
		float cameraFar = s_Data.CameraDataBuffer.FarClip;

		const float splitWeight = s_Data.ShadowSettings.ExponentialSplitFactor;

		for (uint32 i = 0; i < ShaderConstants::SHADOW_CASCADES_COUNT; ++i)
		{
			float percent = (i + 1) / float(ShaderConstants::SHADOW_CASCADES_COUNT);
			float log = cameraNear * Math::Pow(cameraFar / cameraNear, percent);
			float uniform = Math::Lerp(cameraNear, cameraFar, percent); 
			float split = Math::Lerp(uniform, log, splitWeight);

			s_Data.ShadowsDataBuffer.CascadeSplits[i].SplitDepth = split;
		}
	}

	void SceneRenderer::ComputeCascadeSpaceMatrices(const DirectionalLight& light)
	{
		float cameraNear = s_Data.CameraDataBuffer.NearClip;
		float cameraFar = s_Data.CameraDataBuffer.FarClip;

		Matrix4 invCamera = Math::Inverse(s_Data.CameraDataBuffer.ViewMatrix * s_Data.CameraDataBuffer.ProjectionMatrix);

		float lastSplit = 0.f;
		float averageFrustumSize = 0.f;

		for (uint32 layer = 0; layer < ShaderConstants::SHADOW_CASCADES_COUNT; ++layer)
		{
			float split = (s_Data.ShadowsDataBuffer.CascadeSplits[layer].SplitDepth - cameraNear) / (cameraFar - cameraNear); // range (0, 1)

			std::array<Vector3, 8> frustumCorners = {
				//Near face
				Vector3{  1.0f,  1.0f, -1.0f },
				Vector3{ -1.0f,  1.0f, -1.0f },
				Vector3{  1.0f, -1.0f, -1.0f },
				Vector3{ -1.0f, -1.0f, -1.0f },

				//Far face
				Vector3{  1.0f,  1.0f, 1.0f },
				Vector3{ -1.0f,  1.0f, 1.0f },
				Vector3{  1.0f, -1.0f, 1.0f },
				Vector3{ -1.0f, -1.0f, 1.0f },
			};

			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				Vector4 cornerWorldSpace = Vector4(frustumCorners[j], 1.f) * invCamera;
				frustumCorners[j] = cornerWorldSpace / cornerWorldSpace.w;
			}

			for (uint32_t j = 0; j < 4; ++j)
			{
				Vector3 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * split);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplit);
			}

			Vector3 frustumCenter = Vector3(0.f);
			for (const auto& corner : frustumCorners)
				frustumCenter += corner;

			frustumCenter /= frustumCorners.size();

			float radius = 0.0f;
			for (uint32 j = 0; j < frustumCorners.size(); ++j)
			{
				float distance = (frustumCorners[j] - frustumCenter).Length();
				radius = Math::Max(radius, distance);
			}
			radius = Math::Ceil(radius * 16.0f) / 16.0f;

			Vector3 maxExtents = Vector3(radius);
			Vector3 minExtents = -maxExtents;

			minExtents.z += s_Data.ShadowSettings.NearPlaneOffset;
			maxExtents.z += s_Data.ShadowSettings.FarPlaneOffset;

			Matrix4 lightView = Math::LookAt(frustumCenter - light.Direction.GetNormalized() * minExtents.z, frustumCenter, Vector3::Up());
			Matrix4 lightProjection = Math::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, maxExtents.z - minExtents.z);

			Matrix4 lightSpace = lightView * lightProjection;
			
			Vector4 shadowOrigin = Vector4(0.f, 0.f, 0.f, 1.f);
			shadowOrigin = shadowOrigin * lightSpace;
			shadowOrigin = shadowOrigin * (s_Data.ShadowMapResolution / 2.f);

			Vector4 roundedOrigin = Math::Round(shadowOrigin);
			Vector4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * (2.f / s_Data.ShadowMapResolution);
			roundOffset.z = 0.f;
			roundOffset.w = 0.f;

			lightProjection[3] += roundOffset;

			s_Data.ShadowsDataBuffer.LightViewProjMatrices[layer] = lightView * lightProjection;
			s_Data.ShadowsDataBuffer.LightViewMatrices[layer] = lightView;

			s_Data.ShadowsDataBuffer.CascadeSplits[layer].LightFrustumPlanes = { minExtents.z, maxExtents.z };

			averageFrustumSize = Math::Max(averageFrustumSize, maxExtents.x - minExtents.x);

			lastSplit = split;
		}

		s_Data.ShadowsDataBuffer.LightSize /= averageFrustumSize;
	}

	void SceneRenderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<TextureCube>& prefilteredMap, Ref<TextureCube>& irradianceMap)
	{
		// Convert EquirectangularHDRMap to Cubemap
		Ref<TextureCube> skybox;
		{
			uint32 width = s_Data.EnvMapResolution;
			uint32 height = s_Data.EnvMapResolution;

			TextureSamplerDescription sampler;
			sampler.MinFilter = TextureFilter::LINEAR;
			sampler.MagFilter = TextureFilter::LINEAR;
			sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			skybox = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, sampler);

			equirectangularHDRMap->Bind();
			skybox->BindAsImage(1);

			Renderer::BindShader("EquirectangularToCubemap");
			Renderer::Dispatch(width, height, 6);

			skybox->SetFilters(TextureFilter::LINEAR_MIPMAP_LINEAR, TextureFilter::LINEAR);
			skybox->GenerateMipMap(ShaderConstants::MAX_SKYBOX_MAP_LOD);
		}

		// Compute Irradiance Map
		{
			uint32 width = s_Data.IrradianceMapResolution;
			uint32 height = s_Data.IrradianceMapResolution;

			TextureSamplerDescription sampler;
			sampler.MinFilter = TextureFilter::LINEAR;
			sampler.MagFilter = TextureFilter::LINEAR;
			sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			irradianceMap = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, sampler);

			skybox->Bind();
			irradianceMap->BindAsImage(1);

			Renderer::BindShader("IrradianceMapConvolution");
			Renderer::Dispatch(width, height, 6);
		}

		// Compute Prefiltered Skybox Map
		{
			uint32 width = s_Data.EnvMapResolution;
			uint32 height = s_Data.EnvMapResolution;

			TextureSamplerDescription sampler;
			sampler.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
			sampler.MagFilter = TextureFilter::LINEAR;
			sampler.Wrap = TextureWrap::CLAMP_TO_EDGE;

			prefilteredMap = TextureCube::Create(TextureFormat::R11F_G11F_B10F, width, height, sampler);
			prefilteredMap->GenerateMipMap(ShaderConstants::MAX_SKYBOX_MAP_LOD);

			Renderer::BindShader("EnvironmentMipFilter");
			skybox->Bind();

			for (uint32 mip = 0; mip < ShaderConstants::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				prefilteredMap->BindAsImage(1, mip);

				uint32 mipWidth = width * Math::Pow(0.5f, (float)mip);
				uint32 mipHeight = height * Math::Pow(0.5f, (float)mip);

				float roughness = (float)mip / (float)(ShaderConstants::MAX_SKYBOX_MAP_LOD - 1);
				s_Data.EnvMapDataBuffer.EnvironmentMapLOD = roughness;
				s_Data.EnvMapConstantBuffer->SetData(&s_Data.EnvMapDataBuffer, sizeof(EnvironmentMapData));

				Renderer::Dispatch(mipWidth, mipHeight, 6);
			}
		}
	}

	Ref<Framebuffer> SceneRenderer::GetEntityIDFramebuffer()
	{
		return s_Data.EntityIDFramebuffer;
	}

	Ref<Framebuffer> SceneRenderer::GetFinalFramebuffer()
	{
		return s_Data.FinalFramebuffer;
	}

	void SceneRenderer::BlitToScreen()
	{
		s_Data.FinalFramebuffer->BlitToScreen();
	}

	const ShadowSettings& SceneRenderer::GetShadowSettings()
	{
		return s_Data.ShadowSettings;
	}

	void SceneRenderer::SetShadowSettings(const ShadowSettings& settings)
	{
		s_Data.ShadowSettings = settings;
	}

	Antialising SceneRenderer::GetAntialiasingMethod()
	{
		return s_Data.AntialisingMethod;
	}

	void SceneRenderer::SetAntialiasingMethod(Antialising method)
	{
		if (s_Data.AntialisingMethod == method)
			return;

		FramebufferDescription hdrFBDesc = s_Data.HDRFramebuffer->GetDescription();
		FramebufferDescription finalFBDesc = s_Data.FinalFramebuffer->GetDescription();

		switch(method)
		{
		case Antialising::NONE:
		{
			hdrFBDesc.Samples = 1;
			finalFBDesc.Samples = 1;
			s_Data.HDRFramebuffer = Framebuffer::Create(hdrFBDesc);
			s_Data.FinalFramebuffer = Framebuffer::Create(finalFBDesc);
			break;
		}
		case Antialising::MSAA_2X:
		{
			hdrFBDesc.Samples = 2;
			finalFBDesc.Samples = 2;
			s_Data.HDRFramebuffer = Framebuffer::Create(hdrFBDesc);
			s_Data.FinalFramebuffer = Framebuffer::Create(finalFBDesc);
			break;
		}
		case Antialising::MSAA_4X:
		{
			hdrFBDesc.Samples = 4;
			finalFBDesc.Samples = 4;
			s_Data.HDRFramebuffer = Framebuffer::Create(hdrFBDesc);
			s_Data.FinalFramebuffer = Framebuffer::Create(finalFBDesc);
			break;
		}
		case Antialising::MSAA_8X:
		{
			hdrFBDesc.Samples = 8;
			finalFBDesc.Samples = 8;
			s_Data.HDRFramebuffer = Framebuffer::Create(hdrFBDesc);
			s_Data.FinalFramebuffer = Framebuffer::Create(finalFBDesc);
			break;
		}
		}

		s_Data.AntialisingMethod = method;
	}

	DebugView SceneRenderer::GetDebugView()
	{
		return s_Data.CurrentDebugView;
	}

	void SceneRenderer::SetDebugView(DebugView view)
	{
		s_Data.CurrentDebugView = view;
	}

	void SceneRenderer::SetRenderQueueLimit(uint32 limit)
	{
		s_Data.GeometryQueue.SetLimit(limit);
	}

	const SceneRenderer::Statistics& SceneRenderer::GetStatistics()
	{
		return s_Data.Stats;
	}

	void SceneRenderer::ResetStats() 
	{
		SceneRenderer2D::ResetStats();

		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.GeometryCount = 0;
		s_Data.Stats.PointLightsCount = 0;
		s_Data.Stats.DirectionalLightsCount = 0;
	}
}
