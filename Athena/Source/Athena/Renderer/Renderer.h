#pragma once

#include "RenderCommand.h"
#include "OrthographicCamera.h"
#include "Shader.h"
#include "Mesh.h"
#include "Material.h"
#include "Environment.h"


namespace Athena
{
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

		static void RenderMesh(const Ref<StaticMesh>& mesh, const Ref<Material>& material = nullptr, const Matrix4& transform = Matrix4::Identity(), int32 entityID = -1);

		static void Clear(const LinearColor& color);
		static Ref<Framebuffer> GetFramebuffer();
		static const BufferLayout& GetVertexBufferLayout();

		static void ReloadShaders();
		static void PreProcessEnvironmentMap(const Ref<Texture2D>& equirectangularHDRMap, const Ref<Environment>& envStorage);

		static inline RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	};
}
