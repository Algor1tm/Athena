#pragma once

#include "atnpch.h"
#include "Core.h"
#include "Athena/Events/Event.h"


namespace Athena
{
	struct WindowDesc
	{
		using EventCallbackFn = std::function<void(Event&)>;

		uint32_t Width = 1280; 
		uint32_t Height = 720;
		bool VSync = true;
		std::string Title = "Athena App";

		EventCallbackFn EventCallback;
	};

	// Interface representing a desktop system based Window
	class ATHENA_API Window
	{
	public:
		virtual ~Window() {}
		
		virtual void OnUpdate() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const WindowDesc::EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() = 0;

		static Scope<Window> Create(const WindowDesc& desc);
	};
}

