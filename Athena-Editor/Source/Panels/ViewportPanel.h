#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Math/Vector.h"

#include "Panels/Panel.h"
#include "ImGuizmoLayer.h"

#include <functional>


namespace Athena
{
	class Framebuffer;


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
		ViewportPanel(std::string_view name, const Ref<EditorContext>& context);

		virtual void OnImGuiRender() override;

		void SetFramebuffer(const Ref<Framebuffer>& framebuffer, uint32 attachmentIndex) 
		{
			m_Description.AttachedFramebuffer = framebuffer; m_Description.AttachmentIndex = attachmentIndex;
		}

		const ViewportDescription& GetDescription() const { return m_Description; }

		template <typename Func>
		void SetDragDropCallback(const Func& callback) { m_DragDropCallback = callback; }

		template <typename Func>
		void SetUIOverlayCallback(const Func& callback) { m_UIOverlayCallback = callback; }

		void SetImGuizmoLayer(const Ref<ImGuizmoLayer>& layer);

	private:
		Ref<ImGuizmoLayer> m_ImGuizmoLayer;
		ViewportDescription m_Description;

		std::function<void()> m_DragDropCallback;
		std::function<void()> m_UIOverlayCallback;
	};
}
