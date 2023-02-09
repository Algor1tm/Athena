#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Application.h"

#include "Athena/Input/Input.h"

#include <GLFW/glfw3.h>


namespace Athena
{
	static inline int AthenaKeyCodeToGLFWKeyCode(Keyboard::Key athenaKeyCode);
	static inline int AthenaMouseCodeToGLFWMouseCode(Mouse::Button athenaMouseCode);


	bool Input::IsKeyPressed(Keyboard::Key keycode)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetKey(window, AthenaKeyCodeToGLFWKeyCode(keycode));
		return state == GLFW_PRESS;
	}

	bool Input::IsMouseButtonPressed(Mouse::Button button)
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		int state = glfwGetMouseButton(window, AthenaMouseCodeToGLFWMouseCode(button));
		return state;
	}

	Vector2 Input::GetMousePosition()
	{
		auto window = reinterpret_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		return Vector2((float)x, float(y));
	}


	static inline int AthenaKeyCodeToGLFWKeyCode(Keyboard::Key athenaKeyCode)
	{
		switch (athenaKeyCode)
		{
		case Keyboard::Space: return 32;
		case Keyboard::Apostrophe: return 39;
		case Keyboard::Comma: return 44;
		case Keyboard::Minus: return 45;
		case Keyboard::Dot: return 46;
		case Keyboard::Slash: return 47;

		case Keyboard::D0: return 48;
		case Keyboard::D1: return 49;
		case Keyboard::D2: return 50;
		case Keyboard::D3: return 51;
		case Keyboard::D4: return 52;
		case Keyboard::D5: return 53;
		case Keyboard::D6: return 54;
		case Keyboard::D7: return 55;
		case Keyboard::D8: return 56;
		case Keyboard::D9: return 57;

		case Keyboard::Semicolon: return 59;
		case Keyboard::Equal: return 61;

		case Keyboard::A: return 65;
		case Keyboard::B: return 66;
		case Keyboard::C: return 67;
		case Keyboard::D: return 68;
		case Keyboard::E: return 69;
		case Keyboard::F: return 70;
		case Keyboard::G: return 71;
		case Keyboard::H: return 72;
		case Keyboard::I: return 73;
		case Keyboard::J: return 74;
		case Keyboard::K: return 75;
		case Keyboard::L: return 76;
		case Keyboard::M: return 77;
		case Keyboard::N: return 78;
		case Keyboard::O: return 79;
		case Keyboard::P: return 80;
		case Keyboard::Q: return 81;
		case Keyboard::R: return 82;
		case Keyboard::S: return 83;
		case Keyboard::T: return 84;
		case Keyboard::U: return 85;
		case Keyboard::V: return 86;
		case Keyboard::W: return 87;
		case Keyboard::X: return 88;
		case Keyboard::Y: return 89;
		case Keyboard::Z: return 90;

		case Keyboard::LeftBracket: return 91;
		case Keyboard::Backslash: return 92;
		case Keyboard::RightBracket: return 93;
		case Keyboard::GraveAccent: return 96;

		case Keyboard::Escape: return 256;
		case Keyboard::Enter: return 257;
		case Keyboard::Tab: return 258;
		case Keyboard::Backspace: return 259;
		case Keyboard::Insert: return 260;
		case Keyboard::Delete: return 261;
		case Keyboard::Right: return 262;
		case Keyboard::Left: return 263;
		case Keyboard::Down: return 264;
		case Keyboard::Up: return 265;
		case Keyboard::PageUp: return 266;
		case Keyboard::PageDown: return 267;
		case Keyboard::Home: return 268;
		case Keyboard::End: return 269;
		case Keyboard::CapsLock: return 280;
		case Keyboard::ScrollLock: return 281;
		case Keyboard::NumLock: return 282;
		case Keyboard::PrintScreen: return 283;
		case Keyboard::Pause: return 284;
		case Keyboard::F1: return 290;
		case Keyboard::F2: return 291;
		case Keyboard::F3: return 292;
		case Keyboard::F4: return 293;
		case Keyboard::F5: return 294;
		case Keyboard::F6: return 295;
		case Keyboard::F7: return 296;
		case Keyboard::F8: return 297;
		case Keyboard::F9: return 298;
		case Keyboard::F10: return 299;
		case Keyboard::F11: return 300;
		case Keyboard::F12: return 301;
		case Keyboard::F13: return 302;
		case Keyboard::F14: return 303;
		case Keyboard::F15: return 304;
		case Keyboard::F16: return 305;
		case Keyboard::F17: return 306;
		case Keyboard::F18: return 307;
		case Keyboard::F19: return 308;
		case Keyboard::F20: return 309;
		case Keyboard::F21: return 310;
		case Keyboard::F22: return 311;
		case Keyboard::F23: return 312;
		case Keyboard::F24: return 313;

		case Keyboard::KP0: return 320;
		case Keyboard::KP1: return 321;
		case Keyboard::KP2: return 322;
		case Keyboard::KP3: return 323;
		case Keyboard::KP4: return 324;
		case Keyboard::KP5: return 325;
		case Keyboard::KP6: return 326;
		case Keyboard::KP7: return 327;
		case Keyboard::KP8: return 328;
		case Keyboard::KP9: return 329;
		case Keyboard::KPDecimal: return 330;
		case Keyboard::KPDivide: return 331;
		case Keyboard::KPMultiply: return 332;
		case Keyboard::KPSubtract: return 333;
		case Keyboard::KPAdd: return 334;

		case Keyboard::LShift: return 340;
		case Keyboard::LCtrl: return 341;
		case Keyboard::LAlt: return 342;
		case Keyboard::LWindows: return 343;
		case Keyboard::RShift: return 344;
		case Keyboard::RCtrl: return 345;
		case Keyboard::RAlt: return 346;
		case Keyboard::RWindows: return 347;
		case Keyboard::Menu: return 348;
		}

		ATN_CORE_ERROR("Failed to match Athena KeyCode with GLFW KeyCode '{0}'", (int)athenaKeyCode);
		return -1;
	}


	static inline int AthenaMouseCodeToGLFWMouseCode(Mouse::Button athenaMouseCode)
	{
		switch (athenaMouseCode)
		{
		case Mouse::Left: return 0;
		case Mouse::Right: return 1;
		case Mouse::Middle: return 2;
		case Mouse::XButton1: return 3;
		case Mouse::XButton2: return 4;
		}

		ATN_CORE_ERROR("Failed to match Athena MouseCode with GLFW MouseCode '{0}'", (int)athenaMouseCode);
		return -1;
	}

}
