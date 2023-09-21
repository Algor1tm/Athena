#pragma once

#include "Athena/Input/Event.h"

#include <sstream>


namespace Athena 
{
	class ATHENA_API WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS_TYPE(WindowClosed)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};


	class ATHENA_API WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32 width, uint32 height)
			: m_Width(width), m_Height(height) {}

		uint32 GetWidth() const { return m_Width; }
		uint32 GetHeight() const { return m_Height; }

		virtual String ToString() const override
		{
			std::stringstream stream;
			stream << "WindowResizedEvent: width = " << m_Width << ", height = " << m_Height;
			return stream.str();
		}

		EVENT_CLASS_TYPE(WindowResized)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)

	private:
		uint32 m_Width, m_Height;
	};


	class ATHENA_API WindowMoveEvent : public Event
	{
	public:
		WindowMoveEvent(int32 posX, int32 posY)
			: m_PosX(posX), m_PosY(posY) {}

		int32 GetPosX() const { return m_PosX; }
		int32 GetPosY() const { return m_PosY; }

		EVENT_CLASS_TYPE(WindowMoved)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)

	private:
		int32 m_PosX, m_PosY;
	};


	class ATHENA_API WindowGainedFocusEvent : public Event
	{
	public: 
		WindowGainedFocusEvent() = default;

		EVENT_CLASS_TYPE(WindowGainedFocus)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};

	class ATHENA_API WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() = default;

		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};

	class ATHENA_API WindowMaximizeEvent : public Event
	{
	public:
		WindowMaximizeEvent() = default;

		EVENT_CLASS_TYPE(WindowMaximized)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};

	class ATHENA_API WindowIconifyEvent : public Event
	{
	public:
		WindowIconifyEvent() = default;

		EVENT_CLASS_TYPE(WindowIconified)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};

	class ATHENA_API WindowRestoreEvent : public Event
	{
	public:
		WindowRestoreEvent() = default;

		EVENT_CLASS_TYPE(WindowRestored)
		EVENT_CLASS_CATEGORY(EventCategoryWindow)
	};
}