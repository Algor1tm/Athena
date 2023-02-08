#include "SceneRenderer.h"

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Renderer2D.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Material.h"


namespace Athena
{
	void SceneRenderer::Init()
	{

	}

	void SceneRenderer::Shutdown()
	{

	}

	void SceneRenderer::Render(Scene* scene, const Matrix4& viewMatrix, const Matrix4& projectionMatrix)
	{
		Renderer2D::BeginScene(viewMatrix * projectionMatrix);

		auto quads = scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor);
		}

		auto circles = scene->GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, scene->GetEnvironment());

		auto staticMeshes = scene->GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);

					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
					Ref<Animation> animation = animator ? animator->GetCurrentAnimation() : nullptr;

					Renderer::Submit(subMeshes[i].VertexBuffer, material, animation, transform.AsMatrix(), (int32)entity);
				}
			}
		}

		Renderer::WaitAndRender();

		Renderer::EndScene();
	}

	void SceneRenderer::RenderEditorScene(Scene* scene, const EditorCamera& camera)
	{
		Matrix4 viewMatrix = camera.GetViewMatrix();
		Matrix4 projectionMatrix = camera.GetProjectionMatrix();

		Matrix4 viewProjection = viewMatrix * projectionMatrix;
		Renderer2D::BeginScene(viewProjection);

		auto quads = scene->GetAllEntitiesWith<TransformComponent, SpriteComponent>();
		for (auto entity : quads)
		{
			auto [transform, sprite] = quads.get<TransformComponent, SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.AsMatrix(), sprite.Texture, sprite.Color, sprite.TilingFactor, (int32)entity);
		}

		auto circles = scene->GetAllEntitiesWith<TransformComponent, CircleComponent>();
		for (auto entity : circles)
		{
			auto [transform, circle] = circles.get<TransformComponent, CircleComponent>(entity);

			Renderer2D::DrawCircle(transform.AsMatrix(), circle.Color, circle.Thickness, circle.Fade, (int32)entity);
		}

		Renderer2D::EndScene();


		Renderer::BeginScene(viewMatrix, projectionMatrix, scene->GetEnvironment());

		auto staticMeshes = scene->GetAllEntitiesWith<TransformComponent, StaticMeshComponent>();
		for (auto entity : staticMeshes)
		{
			auto [transform, meshComponent] = staticMeshes.get<TransformComponent, StaticMeshComponent>(entity);

			if (!meshComponent.Hide)
			{
				const auto& subMeshes = meshComponent.Mesh->GetAllSubMeshes();
				for (uint32 i = 0; i < subMeshes.size(); ++i)
				{
					Ref<Material> material = MaterialManager::GetMaterial(subMeshes[i].MaterialName);

					Ref<Animator> animator = meshComponent.Mesh->GetAnimator();
					Ref<Animation> animation = animator ? animator->GetCurrentAnimation() : nullptr;

					Renderer::Submit(subMeshes[i].VertexBuffer, material, animation, transform.AsMatrix(), (int32)entity);
				}
			}
		}

		Renderer::WaitAndRender();

		Renderer::EndScene();
	}
}
