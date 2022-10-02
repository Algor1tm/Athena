#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Framebuffer.h"

#include "Panel.h"

#include "ImGuizmoLayer.h"

#include <functional>


namespace Athena
{
	struct ViewportDescription
	{
		bool IsFocused = false;
		bool IsHovered = false;
		Vector2u Size = { 0, 0 };
		Vector2 Bounds[2] = {};

		Ref<Framebuffer> AttachedFramebuffer;
		uint32 AttachmentIndex;
	};

	class ViewportPanel: public Panel
	{
	public:
		ViewportPanel(std::string_view name);

		virtual void OnImGuiRender() override;

		void SetFramebuffer(const Ref<Framebuffer>& framebuffer, uint32 attachmentIndex) 
		{
			m_Description.AttachedFramebuffer = framebuffer; m_Description.AttachmentIndex = attachmentIndex;
		}

		const ViewportDescription& GetDescription() const { return m_Description; }

		template <typename Func>
		void SetDragDropCallback(const Func& callback) { m_DragDropCallback = callback; }

		void SetImGuizmoLayer(ImGuizmoLayer* layer) { m_pImGuizmoLayer = layer; m_pImGuizmoLayer->m_pViewportPanel = this; }

	private:
		ImGuizmoLayer* m_pImGuizmoLayer;
		ViewportDescription m_Description;

		std::function<void()> m_DragDropCallback;
	};
}
