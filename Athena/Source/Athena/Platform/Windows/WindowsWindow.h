#pragma once

#include "Athena/Core/Log.h"
#include "Athena/Core/Window.h"
#include "Athena/Input/Events/ApplicationEvent.h"
#include "Athena/Input/Events/KeyEvent.h"
#include "Athena/Input/Events/MouseEvent.h"

#include <Windows.h>
#include <Windowsx.h>

#include <ImGui/backends/imgui_impl_win32.h>
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


namespace Athena
{
	static constexpr const wchar_t* s_ClassName = L"AthenaApp";
	static HINSTANCE s_HInstance;


	static inline Keyboard::Key WinKeyCodeToAthenaKeyCode(int winKeyCode)
	{
		return static_cast<Keyboard::Key>(winKeyCode);
	}

	static inline Mouse::Button WinMouseCodeToAthenaMouseCode(int winMouseCode)
	{
		return static_cast<Mouse::Button>(winMouseCode);
	}


	static LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;


		switch (msg)
		{
			//------------------WINDOW------------------------
		case WM_CLOSE:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			WindowCloseEvent event;
			data.EventCallback(event);
			return 0;
		}
		case WM_SIZE:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			if (wParam == SIZE_MINIMIZED)
			{
				data.Width = 0;
				data.Height = 0;
			}
			else
			{
				data.Width = LOWORD(lParam);
				data.Height = HIWORD(lParam);
			}

			WindowResizeEvent event(data.Width, data.Height);
			data.EventCallback(event);
			return 0;
		}
			//------------------KEYBOARD------------------------
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			unsigned int isRepeat = lParam & (1 << 30);
			KeyPressedEvent event(WinKeyCodeToAthenaKeyCode((int)wParam), isRepeat);
			data.EventCallback(event);
			return 0;
		}
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			KeyReleasedEvent event(WinKeyCodeToAthenaKeyCode((int)wParam));
			data.EventCallback(event);
			return 0;
		}
		case WM_CHAR:
		case WM_SYSCHAR:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			KeyTypedEvent event(WinKeyCodeToAthenaKeyCode((int)wParam));
			data.EventCallback(event);
			return 0;
		}

		//------------------MOUSE BUTTONS------------------------
		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			Mouse::Button button;
			if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = Mouse::Left; }
			else if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = Mouse::Right; }
			else if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = Mouse::Middle; }
			else if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Mouse::XButton1 : Mouse::XButton2; }

			MouseButtonPressedEvent event(button);
			data.EventCallback(event);
			return 0;
		}

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			Mouse::Button button;
			if (msg == WM_LBUTTONUP) { button = Mouse::Left; }
			else if (msg == WM_RBUTTONUP) { button = Mouse::Right; }
			else if (msg == WM_MBUTTONUP) { button = Mouse::Middle; }
			else if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? Mouse::XButton1 : Mouse::XButton2; }

			MouseButtonReleasedEvent event(button);
			data.EventCallback(event);
			return 0;
		}
		//------------------MOUSE MOVE------------------------
		case WM_MOUSEWHEEL:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			MouseScrolledEvent event(0.f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
			data.EventCallback(event);
			return 0;
		}
		case WM_MOUSEHWHEEL:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			MouseScrolledEvent event((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.f);
			data.EventCallback(event);
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			Window::WindowData& data = *reinterpret_cast<Window::WindowData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

			MouseMovedEvent event((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
			data.EventCallback(event);
			return 0;
		}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	static LRESULT WndProcSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_NCCREATE)
		{
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window::WindowData* data = reinterpret_cast<Window::WindowData*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));

			WndProc(hWnd, msg, wParam, lParam);
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}


	Scope<Window> Window::Create(const WindowDescription& desc)
	{
		Scope<Window> window = CreateScope<Window>();

		WindowData windowData;
		windowData.Width = desc.Width;
		windowData.Height = desc.Height;
		windowData.VSync = desc.VSync;
		windowData.Title = desc.Title;
		windowData.EventCallback = desc.EventCallback;

		window->m_Data = windowData;

		if (m_WindowCount == 0)
		{
			HANDLE icon;
			if (desc.Icon != Filepath())
			{
				std::wstring iconPath = desc.Icon.wstring() + L".ico";		// TODO: Make more customizable
				icon = LoadImage(NULL, iconPath.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
				ATN_CORE_ASSERT(icon, "Failed to load icon");
			}
			else
			{
				icon = nullptr;
			}

			s_HInstance = GetModuleHandle(NULL);
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.style = CS_OWNDC;
			wcex.lpfnWndProc = WndProcSetup;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = s_HInstance;
			wcex.hIcon = reinterpret_cast<HICON>(icon);
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);;
			wcex.hbrBackground = nullptr;
			wcex.lpszMenuName = nullptr;
			wcex.lpszClassName = s_ClassName;
			wcex.hIconSm = nullptr;

			bool result = RegisterClassEx(&wcex);
			ATN_CORE_ASSERT(result, "Failed to Register Windows App");

			ATN_CORE_INFO("Register Windows App");
		}

		RECT wr;
		wr.left = 100;
		wr.right = window->m_Data.Width + wr.left;
		wr.top = 100;
		wr.bottom = window->m_Data.Height + wr.top;
		AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

		std::wstring wtitle;
		wtitle.resize(window->m_Data.Title.size());
		MultiByteToWideChar(CP_UTF8, 0, window->m_Data.Title.c_str(), (int)window->m_Data.Title.size(), &wtitle[0], (int)wtitle.size());

		HWND hWnd;
		window->m_WindowHandle = hWnd = CreateWindow(
			s_ClassName,
			wtitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			100,
			100,
			wr.right - wr.left,
			wr.bottom - wr.top,
			nullptr,
			nullptr,
			s_HInstance,
			&window->m_Data);

		m_WindowCount++;

		ATN_CORE_INFO("Create Windows Window '{0}' ({1}, {2})", window->m_Data.Title, window->m_Data.Width, window->m_Data.Height);

		ShowWindow(hWnd, desc.Mode == WindowMode::Maximized ? SW_SHOWMAXIMIZED : SW_SHOWDEFAULT);
		UpdateWindow(hWnd);
		
		if (desc.Mode == WindowMode::Fullscreen)
		{
			WINDOWPLACEMENT wpPrev = { sizeof(wpPrev) };

			DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
			if (dwStyle & WS_OVERLAPPEDWINDOW)
			{
				MONITORINFO mi = { sizeof(mi) };
				if (GetWindowPlacement(hWnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
				{
					SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
					SetWindowPos(hWnd, HWND_TOP,
						mi.rcMonitor.left, mi.rcMonitor.top,
						mi.rcMonitor.right - mi.rcMonitor.left,
						mi.rcMonitor.bottom - mi.rcMonitor.top,
						SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
				}
			}
		}

		window->m_Context = GraphicsContext::Create(window->m_WindowHandle);
		window->SetVSync(window->m_Data.VSync);

		if (desc.Mode == WindowMode::Maximized)
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			window->m_Data.Width = rect.right - rect.left;
			window->m_Data.Height = rect.bottom - rect.top;
		}

		return window;
	}

	Window::~Window()
	{
		DestroyWindow(reinterpret_cast<HWND>(m_WindowHandle));
		--m_WindowCount;

		ATN_CORE_INFO("Shutdown Windows Window '{0}'", m_Data.Title);

		if (m_WindowCount <= 0)
		{
			UnregisterClass(s_ClassName, s_HInstance);
			ATN_CORE_INFO("Unregister Windows App");
		}
	}

	void Window::OnUpdate()
	{
		MSG msg;

		while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		m_Context->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{
		m_Context->SetVSync(enabled);
		m_Data.VSync = enabled;
	}
}
