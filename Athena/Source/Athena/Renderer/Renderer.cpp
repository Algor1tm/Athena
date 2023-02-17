#include "Renderer.h"

#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/GPUBuffers.h"
#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Texture.h"

#include "Athena/Math/Projections.h"
#include "Athena/Math/Transforms.h"

#include "Athena/Scene/SceneRenderer.h"

#include <deque>


namespace Athena
{
	enum ShaderEnum
	{
		PBR_STATIC,
		PBR_ANIM,
		SKYBOX,

		COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP,
		COMPUTE_IRRADIANCE_MAP,
		COMPUTE_PREFILTER_MAP,

		DEBUG_NORMALS_STATIC,
		DEBUG_NORMALS_ANIM,
		DEBUG_WIREFRAME_STATIC,
		DEBUG_WIREFRAME_ANIM
	};

	static std::unordered_map<ShaderEnum, String> s_ShaderMap
	{
		{ PBR_STATIC, "PBR_STATIC" },
		{ PBR_ANIM, "PBR_ANIM" },
		{ SKYBOX, "SKYBOX" },
		  
		{ COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP, "COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP" },
		{ COMPUTE_IRRADIANCE_MAP, "COMPUTE_IRRADIANCE_MAP" },
		{ COMPUTE_PREFILTER_MAP, "COMPUTE_PREFILTER_MAP" },
		  
		{ DEBUG_NORMALS_STATIC, "DEBUG_NORMALS_STATIC" },
		{ DEBUG_NORMALS_ANIM, "DEBUG_NORMALS_ANIM" },
		{ DEBUG_WIREFRAME_STATIC, "DEBUG_WIREFRAME_STATIC" },
		{ DEBUG_WIREFRAME_ANIM, "DEBUG_WIREFRAME_ANIM" }
	};

	struct DrawCallInfo
	{
		Ref<VertexBuffer> VertexBuffer;
		Ref<Material> Material;
		Ref<Animator> Animator;
		Matrix4 Transform;
		int32 EntityID;
	};

	struct SceneData
	{
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		Vector4 CameraPosition;
		float SkyboxLOD = 0;
		float Exposure = 1;
	};

	struct EntityData
	{
		Matrix4 TransformMatrix;
		int32 EntityID = -1;
	};

	struct LightData
	{
		DirectionalLight DirectionalLightBuffer[ShaderLimits::MAX_DIRECTIONAL_LIGHT_COUNT];
		uint32 DirectionalLightCount = 0;

		PointLight PointLightBuffer[ShaderLimits::MAX_POINT_LIGHT_COUNT];
		uint32 PointLightCount = 0;
	};

	struct RendererData
	{
		std::deque<DrawCallInfo> RenderQueue;	// TODO: maybe make class RenderQueue
		uint32 RenderQueueLastSize = 0;
		int32 RenderQueueLimit = -1;

		Ref<Framebuffer> MainFramebuffer;

		Ref<VertexBuffer> CubeVertexBuffer;
		Ref<Texture2D> BRDF_LUT;

		Ref<Environment> ActiveEnvironment;

		SceneData SceneDataBuffer;
		EntityData EntityDataBuffer;
		LightData LightDataBuffer;

		Ref<ConstantBuffer> SceneConstantBuffer;
		Ref<ConstantBuffer> EntityConstantBuffer;
		Ref<ConstantBuffer> MaterialConstantBuffer;
		Ref<ShaderStorageBuffer> LightShaderStorageBuffer;
		Ref<ShaderStorageBuffer> BoneTransformsShaderStorageBuffer;

		Renderer::Statistics Stats;
		DebugView CurrentDebugView = DebugView::NONE;

		ShaderLibrary ShaderPack;
		void BindShader(ShaderEnum shader) { ShaderPack.Get<Shader>(s_ShaderMap[shader])->Bind(); }

		const uint32 SkyboxSize = 2048;
		const uint32 IrradianceMapSize = 128;
		const uint32 BRDF_LUTSize = 512;
	};

	static RendererData s_Data;

	static void RenderGeometryPass(ShaderEnum staticShader, ShaderEnum animShader)
	{
		if (s_Data.RenderQueueLastSize != s_Data.RenderQueue.size())
			s_Data.RenderQueueLimit = s_Data.RenderQueue.size();
		if (s_Data.RenderQueueLimit < 0)
			s_Data.RenderQueueLimit = s_Data.RenderQueue.size();
		auto end = s_Data.RenderQueueLimit < s_Data.RenderQueue.size() ? s_Data.RenderQueue.begin() + s_Data.RenderQueueLimit : s_Data.RenderQueue.end();

		auto iter = s_Data.RenderQueue.begin();
		// Render Static Meshes
		s_Data.BindShader(staticShader);

		Ref<Material> lastMaterial = nullptr;
		while (iter != end)
		{
			auto& info = *iter;

			if (info.Animator != nullptr)
				break;

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (lastMaterial == nullptr || *lastMaterial != *info.Material)
			{
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));
				lastMaterial = info.Material;
			}

			RenderCommand::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
			iter++;
		}

		// Render Animated Meshes
		if (iter != end)
			s_Data.BindShader(animShader);

		Ref<Animator> lastAnimator;
		while (iter != end)
		{
			auto& info = *iter;

			s_Data.EntityDataBuffer.TransformMatrix = info.Transform;
			s_Data.EntityDataBuffer.EntityID = info.EntityID;
			s_Data.EntityConstantBuffer->SetData(&s_Data.EntityDataBuffer, sizeof(EntityData));

			if (lastAnimator == nullptr || lastAnimator != info.Animator)
			{
				const auto& boneTransforms = info.Animator->GetBoneTransforms();
				s_Data.BoneTransformsShaderStorageBuffer->SetData(boneTransforms.data(), sizeof(Matrix4) * boneTransforms.size());
			}

			if (lastMaterial == nullptr || *lastMaterial != *info.Material)
			{
				s_Data.MaterialConstantBuffer->SetData(&info.Material->Bind(), sizeof(Material::ShaderData));
				lastMaterial = info.Material;
			}

			RenderCommand::DrawTriangles(info.VertexBuffer);
			s_Data.Stats.DrawCalls++;
			iter++;
		}
	}

	void Renderer::Init(RendererAPI::API graphicsAPI)
	{
		RendererAPI::SetAPI(graphicsAPI);
		RenderCommand::Init();
		Renderer2D::Init();
		SceneRenderer::Init();

		FramebufferDescription fbDesc;
		fbDesc.Attachments = { TextureFormat::RGBA8, TextureFormat::RED_INTEGER, TextureFormat::DEPTH24STENCIL8 };
		fbDesc.Width = 1280;
		fbDesc.Height = 720;
		fbDesc.Samples = 4;

		s_Data.MainFramebuffer = Framebuffer::Create(fbDesc);

		s_Data.ShaderPack.Load<IncludeShader>("PBR_FS", "Assets/Shaders/PBR_FS");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[PBR_STATIC], "Assets/Shaders/PBR_Static");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[PBR_ANIM], "Assets/Shaders/PBR_Anim");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[SKYBOX], "Assets/Shaders/Skybox");

		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_EQUIRECTANGULAR_TO_CUBEMAP], "Assets/Shaders/ComputeEquirectangularToCubemap");
		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_IRRADIANCE_MAP], "Assets/Shaders/ComputeIrradianceMap");
		s_Data.ShaderPack.Load<ComputeShader>(s_ShaderMap[COMPUTE_PREFILTER_MAP], "Assets/Shaders/ComputePrefilteredMap");
							  
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_NORMALS_STATIC], "Assets/Shaders/Debug/Normals_Static");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_NORMALS_ANIM], "Assets/Shaders/Debug/Normals_Anim");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_WIREFRAME_STATIC], "Assets/Shaders/Debug/Wireframe_Static");
		s_Data.ShaderPack.Load<Shader>(s_ShaderMap[DEBUG_WIREFRAME_ANIM], "Assets/Shaders/Debug/Wireframe_Anim");

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
		s_Data.LightShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(LightData), BufferBinder::LIGHT_DATA);
		s_Data.BoneTransformsShaderStorageBuffer = ShaderStorageBuffer::Create(sizeof(Matrix4) * ShaderLimits::MAX_NUM_BONES, BufferBinder::BONES_DATA);

		// Compute BRDF_LUT
		uint32 width = s_Data.BRDF_LUTSize;
		uint32 height = s_Data.BRDF_LUTSize;

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
		SceneRenderer::Shutdown();
	}

	void Renderer::OnWindowResized(uint32 width, uint32 height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
	}

	void Renderer::BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const Ref<Environment>& environment)
	{
		s_Data.ActiveEnvironment = environment;

		s_Data.SceneDataBuffer.ViewMatrix = viewMatrix;
		s_Data.SceneDataBuffer.ProjectionMatrix = projectionMatrix;
		s_Data.SceneDataBuffer.CameraPosition = Math::AffineInverse(viewMatrix)[3];
		s_Data.SceneDataBuffer.Exposure = s_Data.ActiveEnvironment->Exposure;
	}

	void Renderer::EndScene()
	{
		Timer timer;

		///////////////////////////// GEOMETRY RENDER PASS /////////////////////////////
		Time start = timer.ElapsedTime();

		if (!s_Data.RenderQueue.empty())
		{
			std::sort(s_Data.RenderQueue.begin(), s_Data.RenderQueue.end(), [](const DrawCallInfo& left, const DrawCallInfo& right)
				{
					if (left.Animator != right.Animator)
						return left.Animator < right.Animator;

					return left.Material->GetName() < right.Material->GetName();
				});

			s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));
			s_Data.LightShaderStorageBuffer->SetData(&s_Data.LightDataBuffer, sizeof(LightData));

			if (s_Data.ActiveEnvironment->Skybox)
				s_Data.ActiveEnvironment->Skybox->Bind();

			s_Data.BRDF_LUT->Bind(TextureBinder::BRDF_LUT);

			RenderGeometryPass(PBR_STATIC, PBR_ANIM);
		}
		s_Data.Stats.GeometryPass = timer.ElapsedTime() - start;

		///////////////////////////// DEBUG VIEW /////////////////////////////
		if (s_Data.CurrentDebugView != DebugView::NONE)
		{
			RenderDebugView(s_Data.CurrentDebugView);
		}

		///////////////////////////// SKYBOX RENDER PASS /////////////////////////////
		start = timer.ElapsedTime();
		if (s_Data.ActiveEnvironment->Skybox)
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

		s_Data.Stats.GeometryCount += s_Data.RenderQueue.size();
		s_Data.Stats.DirectionalLightsCount += s_Data.LightDataBuffer.DirectionalLightCount;
		s_Data.Stats.PointLightsCount += s_Data.LightDataBuffer.PointLightCount;


		s_Data.RenderQueueLastSize = s_Data.RenderQueue.size();
		s_Data.RenderQueue.clear();
		s_Data.ActiveEnvironment = nullptr;

		s_Data.LightDataBuffer.DirectionalLightCount = 0;
		s_Data.LightDataBuffer.PointLightCount = 0;
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

	void Renderer::SubmitLight(const DirectionalLight& dirLight)
	{
		if (ShaderLimits::MAX_DIRECTIONAL_LIGHT_COUNT == s_Data.LightDataBuffer.DirectionalLightCount)
		{
			ATN_CORE_WARN("Renderer::SubmitLight: Attempt to submit more than {} DirectionalLights!", ShaderLimits::MAX_DIRECTIONAL_LIGHT_COUNT);
			return;
		}

		s_Data.LightDataBuffer.DirectionalLightBuffer[s_Data.LightDataBuffer.DirectionalLightCount] = dirLight;
		s_Data.LightDataBuffer.DirectionalLightCount++;
	}

	void Renderer::SubmitLight(const PointLight& pointLight)
	{
		if (ShaderLimits::MAX_POINT_LIGHT_COUNT == s_Data.LightDataBuffer.PointLightCount)
		{
			ATN_CORE_WARN("Renderer::SubmitLight: Attempt to submit more than {} PointLights!", ShaderLimits::MAX_DIRECTIONAL_LIGHT_COUNT);
			return;
		}

		s_Data.LightDataBuffer.PointLightBuffer[s_Data.LightDataBuffer.PointLightCount] = pointLight;
		s_Data.LightDataBuffer.PointLightCount++;
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

			s_Data.RenderQueue.push_back(info);
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

			s_Data.RenderQueue.push_back(info);
		}
		else
		{
			ATN_CORE_WARN("Renderer::Submit(): Attempt to submit nullptr vertexBuffer!");
		}
	}

	void Renderer::Clear(const LinearColor& color)
	{
		RenderCommand::Clear(color);
	}

	Ref<Framebuffer> Renderer::GetMainFramebuffer()
	{
		return s_Data.MainFramebuffer;
	}

	void Renderer::ReloadShaders()
	{
		Renderer2D::ReloadShaders();

		s_Data.ShaderPack.Reload();
	}

	void Renderer::PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<Cubemap>& prefilteredMap, Ref<Cubemap>& irradianceMap)
	{
		// Convert EquirectangularHDRMap to Cubemap
		Ref<Cubemap> skybox;
		{
			uint32 width = s_Data.SkyboxSize;
			uint32 height = s_Data.SkyboxSize;

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
			skybox->GenerateMipMap(ShaderLimits::MAX_SKYBOX_MAP_LOD);
		}

		// Compute Irradiance Map
		{
			uint32 width = s_Data.IrradianceMapSize;
			uint32 height = s_Data.IrradianceMapSize;

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
			uint32 width = s_Data.SkyboxSize;
			uint32 height = s_Data.SkyboxSize;

			CubemapDescription cubeMapDesc;
			cubeMapDesc.Width = width;
			cubeMapDesc.Height = height;
			cubeMapDesc.Format = TextureFormat::R11F_G11F_B10F;
			cubeMapDesc.MinFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;

			prefilteredMap = Cubemap::Create(cubeMapDesc);
			prefilteredMap->GenerateMipMap(ShaderLimits::MAX_SKYBOX_MAP_LOD);

			auto prefilteredCompute = s_Data.ShaderPack.Get<ComputeShader>(s_ShaderMap[COMPUTE_PREFILTER_MAP]);
			prefilteredCompute->Bind();

			skybox->Bind();

			for (uint32 mip = 0; mip < ShaderLimits::MAX_SKYBOX_MAP_LOD; ++mip)
			{
				prefilteredMap->BindAsImage(1, mip);

				uint32 mipWidth = width * Math::Pow(0.5f, (float)mip);
				uint32 mipHeight = height * Math::Pow(0.5f, (float)mip);

				float roughness = (float)mip / (float)(ShaderLimits::MAX_SKYBOX_MAP_LOD - 1);
				s_Data.SceneDataBuffer.SkyboxLOD = roughness;
				s_Data.SceneConstantBuffer->SetData(&s_Data.SceneDataBuffer, sizeof(SceneData));

				prefilteredCompute->Execute(mipWidth, mipHeight, 6);
			}
		}
	}

	void Renderer::RenderDebugView(DebugView view)
	{
		if (s_Data.RenderQueue.size() > 0)
		{
			if (view == DebugView::NORMALS)
				RenderGeometryPass(DEBUG_NORMALS_STATIC, DEBUG_NORMALS_ANIM);

			else if (view == DebugView::WIREFRAME)
				RenderGeometryPass(DEBUG_WIREFRAME_STATIC, DEBUG_WIREFRAME_ANIM);
		}
	}

	void Renderer::SetRenderQueueLimit(uint32 limit)
	{
		s_Data.RenderQueueLimit = limit;
	}

	void Renderer::SetDebugView(DebugView view)
	{
		s_Data.CurrentDebugView = view;
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
