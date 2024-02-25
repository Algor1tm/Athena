#include "DrawList.h"

#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Renderer.h"
#include "Athena/Math/Common.h"


namespace Athena
{
	DrawList::DrawList(bool isAnimated) 
	{
		m_IsAnimated = isAnimated;
	}

	void DrawList::Push(const DrawCall& drawCall)
	{
		m_Array.push_back(drawCall);
	}

	void DrawList::Clear()
	{
		m_LastMaterial = nullptr;
		m_Array.clear();
	}

	void DrawList::Sort()
	{
		std::sort(m_Array.begin(), m_Array.end(), [](const DrawCall& left, const DrawCall& right)
		{
			return left.Material->GetName() < right.Material->GetName();
		});
	}

	void DrawList::Flush(const Ref<Pipeline>& pipeline, bool shadowPass)
	{
		auto commandBuffer = Renderer::GetRenderCommandBuffer();

		for (const auto& drawCall : m_Array)
		{
			if (shadowPass && !drawCall.Material->GetFlag(MaterialFlag::CAST_SHADOWS))
				continue;

			if (!shadowPass && UpdateMaterial(drawCall))
				drawCall.Material->Bind(commandBuffer);

			if(m_IsAnimated)
				drawCall.Material->Set("u_BonesOffset", drawCall.BonesOffset);

			drawCall.Material->Set("u_Transform", drawCall.Transform);
			Renderer::RenderGeometry(commandBuffer, pipeline, drawCall.VertexBuffer, drawCall.Material);
		}

		m_LastMaterial = nullptr;
	}

	bool DrawList::UpdateMaterial(const DrawCall& drawCall)
	{
		if (drawCall.Material != m_LastMaterial)
		{
			m_LastMaterial = drawCall.Material;
			return true;
		}

		return false;
	}
}
