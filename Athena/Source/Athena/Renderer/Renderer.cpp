#include "Renderer.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/RenderQueue.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"


namespace Athena
{
	static std::unordered_map<ShaderEnum, String> s_ShaderMap
	{
		{ PBR, "PBR" },
		{ SHADOW_MAP_GEN, "SHADOW_MAP_GEN" },
		{ SKYBOX, "SKYBOX" },

		{ COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP, "COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP" },
		{ COMPUTE_IRRADIANCE_MAP, "COMPUTE_IRRADIANCE_MAP" },
		{ COMPUTE_PREFILTER_MAP, "COMPUTE_PREFILTER_MAP" },

		{ DEBUG_NORMALS, "DEBUG_NORMALS" },
		{ DEBUG_WIREFRAME, "DEBUG_WIREFRAME" },
		{ DEBUG_SHOW_CASCADES, "DEBUG_SHOW_CASCADES" },
	};

	struct SceneData
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		Vector4 CameraPosition;
		float NearClip;
		float FarClip;
		float SkyboxLOD = 0;
		float Exposure = 1;
	};

	struct EntityData
	{
		Matrix4 TransformMatrix;
		int32 EntityID = -1;
		bool Animated = false;
	};

	struct LightData
	{
		Matrix4 DirectionalLightSpaceMatrices[ShaderConstants::SHADOW_CASCADES_COUNT];
		DirectionalLight DirectionalLightBuffer[ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLightBuffer[ShaderConstants::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;
	};

	struct ShadowsData
	{
		float CascadeSplits[ShaderConstants::SHADOW_CASCADES_COUNT];
		float MaxDistance = 200.f;
		float FadeOut = 10.f;
		float LightSize = 0.5f;
		bool SoftShadows = true;
	};

	struct RendererData
	{
		Ref<Framebuffer> MainFramebuffer;
		Ref<Framebuffer> ShadowMap;

		RenderQueue GeometryQueue;

		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<Texture2D> BRDF_LUT;

		Ref<Environment> ActiveEnvironment;

		SceneData SceneDataBuffer;
		EntityData EntityDataBuffer;
		LightData LightDataBuffer;
		ShadowsData ShadowsDataBuffer;

		Ref<ConstantBuffer> SceneConstantBuffer;
		Ref<ConstantBuffer> EntityConstantBuffer;
		Ref<ConstantBuffer> MaterialConstantBuffer;
		Ref<ConstantBuffer> ShadowsConstantBuffer;
		Ref<ShaderStorageBuffer> LightShaderStorageBuffer;
		Ref<ShaderStorageBuffer> BoneTransformsShaderStorageBuffer;

		ShadowSettings ShadowSettings;

		Antialising AntialisingMethod = Antialising::MSAA_4X;

		DebugView CurrentDebugView = DebugView::NONE;
		Renderer::Statistics Stats;

		ShaderLibrary ShaderPack;
		void BindShader(ShaderEnum shader) { ShaderPack.Get<Shader>(s_ShaderMap[shader])->Bind(); }

		const uint32 SkyboxResolution = 2048;
		const uint32 IrradianceMapResolution = 128;
		const uint32 BRDF_LUTResolution = 512;

		const uint32 ShadowMapResolution = 2048;
	};

	static RendererData s_Data;


	void Renderer::Init(RendererAPI::API graphicsAPI)
	{
		RendererAPI::SetAPI(graphicsAPI);
		RenderCommand::Init();
		Renderer2D::Init();

		FramebufferDescription fbDesc;
		fbDesc.Attachments = { TextureFormat::RGBA8, TextureFormat::RED_INTEGER, TextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Samples = 4;

		s_Data.MainFramebuffer = Framebuffer::Create(fbDesc);

		fbDesc.Attachments = { TextureFormat::DEPTH32F };
		fbDesc.Width = s_Data.ShadowMapResolution;
		fbDesc.Height = s_Data.ShadowMapResolution;
		fbDesc.Depth = ShaderConstants::SHADOW_CASCADES_COUNT;
		fbDesc.Samples = 1;

		s_Data.ShadowMap = Framebuffer::Create(fbDesc);


		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[PBR], "Assets/Shaders/PBR");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[SHADOW_MAP_GEN], "Assets/Shaders/ShadowMap");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[SKYBOX], "Assets/Shaders/Skybox");

		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP], "Assets/Shaders/ComputeEquirectangularToCubemap");
		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_IRRADIANCE_MAP], "Assets/Shaders/ComputeIrradianceMap");
		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_PREFILTER_MAP], "Assets/Shaders/ComputePrefilteredMap");

		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_NORMALS], "Assets/Shaders/Debug/Normals");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_WIREFRAME], "Assets/Shaders/Debug/Wireframe");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_SHOW_CASCADES], "Assets/Shaders/Debug/ShowCascades");


		uint32 cubeIndices[] = { 1, 6, 2, 6, 1, 5,  0, 7, 4, 7, 0, 3,  4, 6, 5, 6, 4, 7,  0, 2, 3, 2, 0, 1,  0, 5, 1, 5, 0, 4,  3, 6, 7, 6, 3, 2 };
		Vector3 cubeVertices[] = { {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, -1.f, -1.f}, {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f}, {1.f, 1.f, 1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f} };

		VertexBufferDescription cubeVBdesc;
		cubeVBdesc.Data = cubeVertices;
		cubeVBdesc.Size = sizeof(cubeVertices);
		cubeVBdesc.Layout = { { ShaderDataType::Float3, "a_Position" } };
		cubeVBdesc.IndexBuffer = IndexBuffer::Create(cubeIndices, std::size(cubeIndices));
		cubeVBdesc.Usage = BufferUsage::STATIC;

		s_Data.CubeVertexBuffer = VertexBuffer::Create(cubeVBdesc);


		s_Data.SceneConstantBuffer = ConstantBuffer::Create(sizeof(SceneData), BufferBinder::SCENE_DATA);
		s_Data.EntityConstantBuffer = ConstantBuffer::Create(sizeof(EntityData), BufferBinder::ENTITY_DATA);
		s_Data.MaterialConstantBuffer = ConstantBuffer::Create(sizeof(Material::ShaderData), BufferBinder::MATERIAL_DATA);
		s_Data.ShadowsConstantBuffer = ConstantBuffer::Create(sizeof(ShadowsData), BufferBinder::SHADOWS_DATA);
		s_Data.LightShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(LightData), BufferBinder::LIGHT_DATA);
		s_Data.BoneTransformsShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(Matrix4) * ShaderConstants::MAX_NUM_BONES, BufferBinder::BONES_DATA);


		// Compute BRDF_LUT
		uint32 width = s_Data.BRDF_LUTResolution;
		uint32 height = s_Data.BRDF_LUTResolution;

		Texture2DDescription brdf_lutDesc;
		brdf_lutDesc.Width = width;
		brdf_lutDesc.Height = height;
		brdf_lutDesc.Format = TextureFormat::RG16F;
		brdf_lutDesc.Wrap = TextureWrap::CLAMP_TO_EDGE;

		s_Data.BRDF_LUT = Texture2D::Create(brdf_lutDesc);

		Ref<ComputeShader> ComputeBRDF_LUTShader = ComputeShader::Create("Assets/Shaders/ComputeBRDF_LUT");
		ComputeBRDF_LUTShader->Bind();

		s_Data.BRDF_LUT->BindAsImage();
		ComputeBRDF_LUTShader->Execute(width, height);

		ComputeBRDF_LUTShader->UnBind();
	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		s_Data.MainFramebuffer->Resize(width, height);
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const CameraInfo& cameraInfo, const Ref<Environment>& environment)
	{
		s_Data.ActiveEnvironment = environment;

		s_Data.SceneDataBuffer.ViewMatrix = cameraInfo.ViewMatrix;
		s_Data.SceneDataBuffer.ProjectionMatrix = cameraInfo.ProjectionMatrix;
		s_Data.SceneDataBuffer.CameraPosition = Math::AffineInverse(cameraInfo.ViewMatrix)[3];
		s_Data.SceneDataBuffer.NearClip = cameraInfo.NearClip;
		s_Data.SceneDataBuffer.FarClip = cameraInfo.FarClip;
		s_Data.SceneDataBuffer.Exposure = s_Data.ActiveEnvironment->Exposure;

		ComputeCascadeSplits();
	}

	void Renderer::EndScene()
	{
		s_Data.GeometryQueue.Sort();

		ShadowMapPass();
		GeometryPass();
		DebugViewPass();
		SkyboxPass();

		s_Data.GeometryQueue.Clear();
		s_Data.ActiveEnvironment = nullptr;

		s_Data.LightDataBuffer.DirectionalLightCount = 0;
		s_Data.LightDataBuffer.PointLightCount = 0;
	}

	void Renderer::SubmitLight(const DirectionalLight& dirLight)
	{
		if (ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT == s_Data.LightDataBuffer.DirectionalLightCount)
		{
			ATN_CORE_WARN("Renderer::SubmitLight: Attempt to submit more than {} DirectionalLights!", ShaderConstants::MAX_DIRECTIONAL_LIGHT_COUNT);
			return;
		}

		uint32 currentIndex = s_Data.LightDataBuffer.DirectionalLightCount;

		s_Data.LightDataBuffer.DirectionalLightBuffer[currentIndex] = dirLight;

		Matrix4 projection = Math::Ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 50.f);
		Matrix4 view = Math::LookAt(dirLight.Direction.GetNormalized() * 7.f, Vector3(0.f), Vector3(0.f, 1.f, 0.f));
		//s_Data.LightDataBuffer.DirectionalLightSpaceMatrices[currentIndex] = view * projection;

		ComputeCascadeSpaceMatrices(dirLight);

		s_Data.LightDataBuffer.DirectionalLightCount++;

		s_Data.Stats.DirectionalLightsCount++;
	}

	void Renderer::SubmitLight(const PointLight& pointLight)
	{
		if (ShaderConstants::MAX_POINT_LIGHT_COUNT == s_Data.LightDataBuffer.PointLightCount)
		{
			ATN_CORE_WARN("Renderer::SubmitLight: Attempt to submit more than {} PointLights!", ShaderConstants::MAX_POINT_LIGHT_COUNT);
			return;
		}

		s_Data.LightDataBuffer.PointLightBuffer[s_Data.LightDataBuffer.PointLightCount] = pointLight;
		s_Data.LightDataBuffer.PointLightCount++;

		s_Data.Stats.PointLightsCount ++;
	}

	void Renderer::Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform, int32 entityID)
	{
		if (vertexBuffer)
		{
			DrawCallInfo info;
			info.VertexBuffer = vertexBuffer;
			info.Material = material;
			info.Transform = transform;
			info.EntityID = entityID;

			s_Data.GeometryQueue.Push(info);
			s_Data.Stats.GeometryCount++;
		}
		else
		{
			ATN_CORE_WARN("Renderer::Submit(): Attempt to submit nullptr vertexBuffer!");
		}
	}

	void Renderer::SubmitWithAnimation(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Ref<Animator>& animator, const Matrix4& transform, int32 entityID)
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
			ATN_CORE_WARN("Renderer::Submit(): Attempt to submit nullptr vertexBuffer!");
		}
	}

	void Renderer::RenderGeometry(ShaderEnum shader, bool useMaterials)
	{
		if (s_Data.GeometryQueue.Empty())
			return;

		s_Data.BindShader(shader);

		// Render Static Meshes
		s_Data.EntityDataBuffer.Animated = false;

		while (s_Data.GeometryQueue.HasStaticMeshes())
		{
			auto& info = s_Data.GeometryQueue.Next();

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (useMaterials && s_Data.GeometryQueue.UpdateMaterial())
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));

			RenderCommand::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
		}

		// Render Animated Meshes
		s_Data.EntityDataBuffer.Animated = true;

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

			RenderCommand::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
		}

		s_Data.GeometryQueue.Reset();
	}

	void Renderer::ShadowMapPass()
	{
		s_Data.ShadowMap->Bind();
		RenderCommand::Clear({ 1, 1, 1, 1 });
		RenderCommand::SetCullMode(CullFace::FRONT);

		s_Data.LightShaderStorageBuffer->SetData(&s_Data.LightDataBuffer, sizeof(LightData));

		for (uint32 i = 0; i < s_Data.LightDataBuffer.DirectionalLightCount; ++i)
		{
			RenderGeometry(SHADOW_MAP_GEN, false);
		}

		RenderCommand::SetCullMode(CullFace::BACK);
	}

	void Renderer::GeometryPass()
	{
		Timer timer;
		Time start = timer.ElapsedTime();

		s_Data.MainFramebuffer->Bind();

		s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));

		s_Data.ShadowsDataBuffer.SoftShadows = s_Data.ShadowSettings.SoftShadows;
		s_Data.ShadowsDataBuffer.MaxDistance = s_Data.ShadowSettings.MaxDistance;
		s_Data.ShadowsDataBuffer.FadeOut = s_Data.ShadowSettings.FadeOut;
		s_Data.ShadowsConstantBuffer->SetData(&s_Data.ShadowsDataBuffer, sizeof(ShadowsData));

		if (s_Data.ActiveEnvironment && s_Data.ActiveEnvironment->Skybox)
			s_Data.ActiveEnvironment->Skybox->Bind();

		s_Data.BRDF_LUT->Bind(TextureBinder::BRDF_LUT);
		s_Data.ShadowMap->BindDepthAttachment(TextureBinder::SHADOW_MAP);

		RenderGeometry(PBR, true);

		s_Data.Stats.GeometryPass = timer.ElapsedTime() - start;
	}

	void Renderer::DebugViewPass()
	{
		if (s_Data.CurrentDebugView == DebugView::NORMALS)
			RenderGeometry(DEBUG_NORMALS, false);

		else if (s_Data.CurrentDebugView == DebugView::WIREFRAME)
			RenderGeometry(DEBUG_WIREFRAME, false);

		else if (s_Data.CurrentDebugView == DebugView::SHOW_CASCADES)
			RenderGeometry(DEBUG_SHOW_CASCADES, false);
	}

	void Renderer::SkyboxPass()
	{
		Timer timer;
		Time start = timer.ElapsedTime();

		if (s_Data.ActiveEnvironment && s_Data.ActiveEnvironment->Skybox)
		{
			s_Data.SceneDataBuffer.SkyboxLOD = s_Data.ActiveEnvironment->SkyboxLOD;

			// Remove translation
			s_Data.SceneDataBuffer.ViewMatrix[3][0] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][1] = 0;
			s_Data.SceneDataBuffer.ViewMatrix[3][2] = 0;

			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));

			s_Data.ActiveEnvironment->Skybox->Bind();

			s_Data.BindShader(SKYBOX);

			RenderCommand::DrawTriangles(s_Data.CubeVertexBuffer);
			s_Data.Stats.DrawCalls++;
		}
		s_Data.Stats.SkyboxPass = timer.ElapsedTime() - start;
	}

	void Renderer::ComputeCascadeSplits()
	{
		float cameraNear = s_Data.SceneDataBuffer.NearClip;
		float cameraFar = s_Data.SceneDataBuffer.FarClip;

		const float splitWeight = s_Data.ShadowSettings.ExponentialSplitFactor;

		for (uint32 i = 0; i < ShaderConstants::SHADOW_CASCADES_COUNT; ++i)
		{
			float percent = (i + 1) / float(ShaderConstants::SHADOW_CASCADES_COUNT);
			float log = cameraNear * Math::Pow(cameraFar / cameraNear, percent);
			float uniform = Math::Lerp(cameraNear, cameraFar, percent); 
			float split = Math::Lerp(uniform, log, splitWeight);

			s_Data.ShadowsDataBuffer.CascadeSplits[i] = split;
		}
	}

	void Renderer::ComputeCascadeSpaceMatrices(const DirectionalLight& light)
	{
		float cameraNear = s_Data.SceneDataBuffer.NearClip;
		float cameraFar = s_Data.SceneDataBuffer.FarClip;

		Matrix4 invCamera = Math::Inverse(s_Data.SceneDataBuffer.ViewMatrix * s_Data.SceneDataBuffer.ProjectionMatrix);
		float lastSplit = 0.f;

		for (uint32 layer = 0; layer < ShaderConstants::SHADOW_CASCADES_COUNT; ++layer)
		{
			float split = (s_Data.ShadowsDataBuffer.CascadeSplits[layer] - cameraNear) / (cameraFar - cameraNear); // range (0, 1)

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

#if 1
			float radius = 0.0f;
			for (uint32 j = 0; j < 8; ++j)
			{
				float distance = (frustumCorners[j] - frustumCenter).Length();
				radius = Math::Max(radius, distance);
			}
			radius = Math::Ceil(radius * 16.0f) / 16.0f;

			Vector3 maxExtents = Vector3(radius);
			Vector3 minExtents = -maxExtents;

			Matrix4 lightView = Math::LookAt(frustumCenter + light.Direction.GetNormalized() * minExtents.z, frustumCenter, Vector3::Up());
			Matrix4 lightProjection = Math::Ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 
				s_Data.ShadowSettings.NearPlaneOffset, maxExtents.z - minExtents.z + s_Data.ShadowSettings.FarPlaneOffset);

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

			s_Data.LightDataBuffer.DirectionalLightSpaceMatrices[layer] = lightView * lightProjection;
			
#else 
			Matrix4 lightView = Math::LookAt(frustumCenter + light.Direction.GetNormalized(), frustumCenter, Vector3::Up());

			Vector3 min = Vector3(Math::MaxValue<float>());
			Vector3 max = Vector3(Math::MinValue<float>());
			for (const auto& corner : frustumCorners)
			{
				const auto transformed = Vector3(corner * lightView);
				min = Math::Min(min, transformed);
				max = Math::Max(max, transformed);
			}

			// Tune this parameter according to the scene
			const float zMult = 10.0f;

			min.z *= min.z < 0 ? zMult : 1.f / zMult;
			max.z *= max.z < 0 ? 1.f / zMult : zMult;

			Matrix4 lightProjection = Math::Ortho(min.x, max.x, min.y, max.y, min.z, max.z);
			Matrix4 lightSpace = lightView * lightProjection;

			s_Data.LightDataBuffer.DirectionalLightSpaceMatrices[layer] = lightSpace;
#endif
			lastSplit = split;
		}
	}

	void Renderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<Cubemap>& prefilteredMap, Ref<Cubemap>& irradianceMap)
	{
		// Convert EquirectangularHDRMap to Cubemap
		Ref<Cubemap> skybox;
		{
			uint32 width = s_Data.SkyboxResolution;
			uint32 height = s_Data.SkyboxResolution;

			CubemapDescription cubeMapDesc;
			cubeMapDesc.Width = width;
			cubeMapDesc.Height = height;
			cubeMapDesc.Format = TextureFormat::R11F_G11F_B10F;
			cubeMapDesc.MinFilter = TextureFilter::LINEAR;

			skybox = Cubemap::Create(cubeMapDesc);

			equirectangularHDRMap->Bind();
			skybox->BindAsImage(1);

			auto equirectangularToCubeMap = s_Data.ShaderPack.Get<ComputeShader>(s_ShaderMap[COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP]);
			equirectangularToCubeMap->Bind();
			equirectangularToCubeMap->Execute(width, height, 6);

			skybox->SetFilters(TextureFilter::LINEAR_MIPMAP_LINEAR, TextureFilter::LINEAR);
			skybox->GenerateMipMap(ShaderConstants::MAX_SKYBOX_MAP_LOD);
		}

		// Compute Irradiance Map
		{
			uint32 width = s_Data.IrradianceMapResolution;
			uint32 height = s_Data.IrradianceMapResolution;

			CubemapDescription cubeMapDesc;
			cubeMapDesc.Width = width;
			cubeMapDesc.Height = height;
			cubeMapDesc.Format = TextureFormat::R11F_G11F_B10F;
			cubeMapDesc.MinFilter = TextureFilter::LINEAR;

			irradianceMap = Cubemap::Create(cubeMapDesc);

			skybox->Bind();
			irradianceMap->BindAsImage(1);

			auto irradianceCompute = s_Data.ShaderPack.Get<ComputeShader>(s_ShaderMap[COMPUTE_IRRADIANCE_MAP]);
			irradianceCompute->Bind();
			irradianceCompute->Execute(width, height, 6);
		}

		// Compute Prefiltered Skybox Map
		{
			uint32 width = s_Data.SkyboxResolution;
			uint32 height = s_Data.SkyboxResolution;

			CubemapDescription cubeMapDesc;
			cubeMapDesc.Width = width;
			cubeMapDesc.Height = height;
			cubeMapDesc.Format = TextureFormat::R11F_G11F_B10F;
			cubeMapDesc.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;

			prefilteredMap = Cubemap::Create(cubeMapDesc);
			prefilteredMap->GenerateMipMap(ShaderConstants::MAX_SKYBOX_MAP_LOD);

			auto prefilteredCompute = s_Data.ShaderPack.Get<ComputeShader>(s_ShaderMap[COMPUTE_PREFILTER_MAP]);
			prefilteredCompute->Bind();

			skybox->Bind();

			for (uint32 mip = 0; mip < ShaderConstants::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				prefilteredMap->BindAsImage(1, mip);

				uint32 mipWidth = width * Math::Pow(0.5f, (float)mip);
				uint32 mipHeight = height * Math::Pow(0.5f, (float)mip);

				float roughness = (float)mip / (float)(ShaderConstants::MAX_SKYBOX_MAP_LOD - 1);
				s_Data.SceneDataBuffer.SkyboxLOD = roughness;
				s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));

				prefilteredCompute->Execute(mipWidth, mipHeight, 6);
			}
		}
	}

	void Renderer::BeginFrame()
	{
		s_Data.MainFramebuffer->Bind();
	}

	void Renderer::EndFrame()
	{
		s_Data.MainFramebuffer->ResolveMutlisampling();
		s_Data.MainFramebuffer->UnBind();
	}

	Ref<Framebuffer> Renderer::GetMainFramebuffer()
	{
		return s_Data.MainFramebuffer;
	}

	void Renderer::BlitToScreen()
	{
		s_Data.MainFramebuffer->BlitToScreen();
	}

	Ref<Framebuffer> Renderer::GetShadowMapFramebuffer()
	{
		return s_Data.ShadowMap;
	}

	void Renderer::ReloadShaders()
	{
		Renderer2D::ReloadShaders();

		s_Data.ShaderPack.Reload();
	}

	const ShadowSettings& Renderer::GetShadowSettings()
	{
		return s_Data.ShadowSettings;
	}

	void Renderer::SetShadowSettings(const ShadowSettings& settings)
	{
		s_Data.ShadowSettings = settings;
	}

	Antialising Renderer::GetAntialiasingMethod()
	{
		return s_Data.AntialisingMethod;
	}

	void Renderer::SetAntialiasingMethod(Antialising method)
	{
		if (s_Data.AntialisingMethod == method)
			return;

		FramebufferDescription desc = s_Data.MainFramebuffer->GetDescription();

		switch(method)
		{
		case Antialising::NONE:
		{
			desc.Samples = 1;
			s_Data.MainFramebuffer = Framebuffer::Create(desc);
			break;
		}
		case Antialising::MSAA_2X:
		{
			desc.Samples = 2;
			s_Data.MainFramebuffer = Framebuffer::Create(desc);
			break;
		}
		case Antialising::MSAA_4X:
		{
			desc.Samples = 4;
			s_Data.MainFramebuffer = Framebuffer::Create(desc);
			break;
		}
		case Antialising::MSAA_8X:
		{
			desc.Samples = 8;
			s_Data.MainFramebuffer = Framebuffer::Create(desc);
			break;
		}
		}

		s_Data.AntialisingMethod = method;
	}

	DebugView Renderer::GetDebugView()
	{
		return s_Data.CurrentDebugView;
	}

	void Renderer::SetDebugView(DebugView view)
	{
		s_Data.CurrentDebugView = view;
	}

	void Renderer::SetRenderQueueLimit(uint32 limit)
	{
		s_Data.GeometryQueue.SetLimit(limit);
	}

	const Renderer::Statistics& Renderer::GetStatistics()
	{
		return s_Data.Stats;
	}

	void Renderer::ResetStats() 
	{
		s_Data.Stats.DrawCalls = 0;
		s_Data.Stats.GeometryCount = 0;
		s_Data.Stats.PointLightsCount = 0;
		s_Data.Stats.DirectionalLightsCount = 0;
	}
}
