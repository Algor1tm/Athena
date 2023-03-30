#pragma once

#include "Athena/Input/Event.h"
#include "Athena/Input/Mouse.h"

#include <sstream>


namespace Athena
{
	class ATHENA_API MouseMovedEvent: public Event
	{
	public:
		MouseMovedEvent(float x, float y)
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


	class ATHENA_API MouseButtonEvent : public Event
	{
	public:
		Mouse::Button GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(Mouse::Button button)
			: m_Button(button) {}

		Mouse::Button m_Button;
	};


	class ATHENA_API MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(Mouse::Button button)
			: MouseButtonEvent(button) {}

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
		MouseButtonReleasedEvent(Mouse::Button button)
			: MouseButtonEvent(button) {}

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "MouseButtonReleasedEvent:" << m_Button;
			return stream.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}
