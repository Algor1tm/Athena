#pragma once

#include "Event.h"
#include "Athena/Core/KeyCodes.h"

#include <sstream>

namespace Athena
{
	class ATHENA_API KeyEvent: public Event
	{
	public:
		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
	protected:
		KeyEvent(KeyCode keycode)
			: m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};


	class ATHENA_API KeyPressedEvent: public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keycode, bool isRepeat)
			: KeyEvent(keycode), m_IsRepeat(isRepeat) {}

		inline bool IsRepeat() const { return m_IsRepeat; }

		String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyPressedEvent: " << m_KeyCode << " (Is repeat - " << m_IsRepeat << ")";
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)
	private:
		bool m_IsRepeat;
	};


	class ATHENA_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keycode)
			: KeyEvent(keycode) {}

		String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyReleasedEvent: " << m_KeyCode;
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)
	};


	class ATHENA_API KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(KeyCode keycode)
			: KeyEvent(keycode) {}


		String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyTypedEvent: " << m_KeyCode;
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}
