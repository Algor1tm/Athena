#pragma once

#include "Athena/Input/Event.h"
#include "Athena/Input/Keyboard.h"

#include <sstream>


namespace Athena
{
	class ATHENA_API KeyEvent: public Event
	{
	public:
		Keyboard::Key GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(Keyboard::Key keycode)
			: m_KeyCode(keycode) {}

		Keyboard::Key m_KeyCode;
	};


	class ATHENA_API KeyPressedEvent: public KeyEvent
	{
	public:
		KeyPressedEvent(Keyboard::Key keycode, bool isRepeat, bool ctrl = false, bool alt = false, bool shift = false)
			: KeyEvent(keycode), m_IsRepeat(isRepeat), 
			m_Ctrl(ctrl), m_Alt(alt), m_Shift(shift) {}

		bool IsRepeat() const { return m_IsRepeat; }

		bool IsCtrlPressed() const { return m_Ctrl; }
		bool IsAltPressed() const { return m_Alt; }
		bool IsShiftPressed() const { return m_Shift; }

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyPressedEvent: " << m_KeyCode << " (Is repeat - " << m_IsRepeat << ")";
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat;
		bool m_Ctrl, m_Alt, m_Shift;
	};


	class ATHENA_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(Keyboard::Key keycode, bool ctrl = false, bool alt = false, bool shift = false)
			: KeyEvent(keycode),
			m_Ctrl(ctrl), m_Alt(alt), m_Shift(shift) {}

		bool IsCtrlPressed() const { return m_Ctrl; }
		bool IsAltPressed() const { return m_Alt; }
		bool IsShiftPressed() const { return m_Shift; }

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyReleasedEvent: " << m_KeyCode;
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyReleased)

	private:
		bool m_Ctrl, m_Alt, m_Shift;
	};


	class ATHENA_API KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(Keyboard::Key keycode)
			: KeyEvent(keycode) {}

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "KeyTypedEvent: " << m_KeyCode;
			return stream.str();
		}

		EVENT_CLASS_TYPE(KeyTyped)
	};
}
