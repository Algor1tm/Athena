#pragma once

#include "Athena/Core/Window.h"
#include "Athena/Renderer/GraphicsContext.h"


struct GLFWwindow;

namespace Athena
{
	class ATHENA_API WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowDESC& desc);
		~WindowsWindow();

		virtual void OnUpdate() override;

		virtual inline uint32 GetWidth() const override { return m_Desc.Width; }
		virtual inline uint32 GetHeight() const override { return m_Desc.Height; }

		virtual inline void SetEventCallback(const WindowDESC::EventCallbackFn& callback) override
		{
			m_Desc.EventCallback = callback;
		}

		virtual void SetVSync(bool enabled) override;
		virtual inline bool IsVSync() const override { return m_Desc.VSync; }

		virtual inline void* GetNativeWindow() override { return m_Window; }

	private:
		void Init();
		void Shutdown();

	private:
		GLFWwindow* m_Window;
		WindowDESC m_Desc;
		Ref<GraphicsContext> m_Context;
	};
}
