#pragma once

#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"
#include "Environment.h"


namespace Athena
{
	enum TextureBinder
	{
		ALBEDO_TEXTURE = 0,
		NORMAL_MAP = 1,
		ROUGHNESS_MAP = 2,
		METALNESS_MAP = 3,
		AMBIENT_OCCLUSION_MAP = 4,

		SKY_BOX = 5,
		IRRADIANCE_MAP = 6,
		BRDF_LUT = 7,
	};

	enum ConstantBufferBinder
	{
		RENDERER2D_CAMERA_DATA = 0,
		SCENE_DATA = 1,
		MATERIAL_DATA = 2,
		LIGHT_DATA = 3
	};

	struct Vertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
	};

	class ATHENA_API Renderer
	{
	public:
		static void Init(RendererAPI::API graphicsAPI);
		static void Shutdown();

		static void OnWindowResized(uint32 width, uint32 height);

		static void BeginScene(const Matrix4& viewMatrix, const Matrix4& projectionMatrix, const Ref<Environment>& environment);
		static void EndScene();

		static void BeginFrame();
		static void EndFrame();

		static void Submit(const Ref<VertexBuffer>& vertexBuffer, const Ref<Material>& material, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);
		static void WaitAndRender();

		static void Clear(const LinearColor& color);
		static Ref<Framebuffer> GetFramebuffer();
		static const BufferLayout& GetVertexBufferLayout();

		static void ReloadShaders();
		static void PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, Ref<Cubemap>& prefilteredMap, Ref<Cubemap>& irradianceMap);

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}
