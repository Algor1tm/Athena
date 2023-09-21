#pragma once

#include "Athena/Input/Event.h"
#include "Athena/Input/Mouse.h"

#include <sstream>


namespace Athena
{
	class ATHENA_API MouseMoveEvent: public Event
	{
	public:
		MouseMoveEvent(float x, float y)
			: m_MouseX(x), m_MouseY(y) {}

		inline float GetX() const { return m_MouseX; }
		inline float GetY() const { return m_MouseY; }

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "MouseMovedEvent: x = " << m_MouseX << ", y = " << m_MouseY;
			return stream.str();
		}

		EVENT_CLASS_TYPE(MouseMoved)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_MouseX, m_MouseY;
	};


	class ATHENA_API MouseButtonEvent : public Event
	{
	public:
		Mouse::Button GetMouseButton() const { return m_Button; }

		bool IsCtrlPressed() const { return m_Ctrl; }
		bool IsAltPressed() const { return m_Alt; }
		bool IsShiftPressed() const { return m_Shift; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(Mouse::Button button, bool ctrl, bool alt, bool shift)
			: m_Button(button), m_Ctrl(ctrl), m_Alt(alt), m_Shift(shift) {}

		Mouse::Button m_Button;
		bool m_Ctrl, m_Alt, m_Shift;
	};


	class ATHENA_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(Mouse::Button button, bool ctrl, bool alt, bool shift)
			: MouseButtonEvent(button, ctrl, alt, shift) {}

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "MouseButtonPressedEvent:" << m_Button;
			return stream.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};


	class ATHENA_API MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(Mouse::Button button, bool ctrl, bool alt, bool shift)
			: MouseButtonEvent(button, ctrl, alt, shift) {}

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "MouseButtonReleasedEvent:" << m_Button;
			return stream.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};


	class ATHENA_API MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {}

		inline float GetXOffset() const { return m_XOffset; }
		inline float GetYOffset() const { return m_YOffset; }

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "MouseScrolledEvent: Xoffset = " << m_XOffset << ", Yoffset = " << m_YOffset;
			return stream.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled)
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset, m_YOffset;
	};

}
