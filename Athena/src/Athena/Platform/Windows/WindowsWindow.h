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

		void OnUpdate() override;

		inline unsigned int GetWidth() const override { return m_Desc.Width; }
		inline unsigned int GetHeight() const override { return m_Desc.Height; }

		inline void SetEventCallback(const WindowDESC::EventCallbackFn& callback) override
		{
			m_Desc.EventCallback = callback;
		}

		void SetVSync(bool enabled) override;
		inline bool IsVSync() const override { return m_Desc.VSync; }

		inline void* GetNativeWindow() override { return m_Window; }

	private:
		virtual void Init();
		virtual void Shutdown();

	private:
		GLFWwindow* m_Window;
		WindowDESC m_Desc;
		Scope<GraphicsContext> m_Context;
	};
}
