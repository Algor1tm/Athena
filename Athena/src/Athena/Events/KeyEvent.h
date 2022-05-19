#pragma once

#include "Event.h"


namespace Athena
{
	class ATHENA_API KeyEvent: public Event
	{
	public:
		inline int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(int keycode)
			: m_KeyCode(keycode) {}

		int m_KeyCode;
	};


	class ATHENA_API KeyPressedEvent: public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode, int repeatCount)
			: KeyEvent(keycode), m_RepeatCount(repeatCount) {}

		inline int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::string result = "KeyPressedEvent: " + std::to_string(m_KeyCode) + " (" + 
								  std::to_string(m_RepeatCount) + "repeats)";
			return result;
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		int m_RepeatCount;
	};


	class ATHENA_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::string result = "KeyReleasedEvent: " + std::to_string(m_KeyCode);
			return result;
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};


	class ATHENA_API KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(int keycode)
			: KeyEvent(keycode) {}


		std::string ToString() const override
		{
			std::string result = "KeyTypedEvent: " + std::to_string(m_KeyCode);
			return result;
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}
