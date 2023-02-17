#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "Panels/Panel.h"

#include <functional>


namespace Athena
{
	class Framebuffer;
	class ImGuizmoLayer;


	struct ViewportDescription
	{
		bool IsFocused = false;
		bool IsHovered = false;
		Vector2u Size = { 0, 0 };
		Vector2 Bounds[2] = {};
		Vector2 Position = { 0, 0 };

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

		void SetImGuizmoLayer(ImGuizmoLayer* layer);

	private:
		ImGuizmoLayer* m_pImGuizmoLayer;
		ViewportDescription m_Description;

		std::function<void()> m_DragDropCallback;
	};
}
