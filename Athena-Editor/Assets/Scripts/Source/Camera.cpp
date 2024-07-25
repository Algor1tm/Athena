#include "Camera.h"


namespace TestProject
{
	// EXPORT_SCRIPT(Camera)

	extern "C"
	{
		SCRIPT_API void _Camera_Instantiate(::Athena::Script** outScript)
		{
			*outScript = new Camera();
		}

		SCRIPT_API void _Camera_OnCreate(::Athena::Script* _this)
		{
			_this->OnCreate();
		}

		SCRIPT_API void _Camera_OnUpdate(::Athena::Script* _this, float frameTime)
		{
			_this->OnUpdate(::Athena::Time::Milliseconds(frameTime));
		}

		SCRIPT_API void _Camera_GetFieldsDescription(::Athena::Script* _this, ScriptFieldMap* outFields)
		{
			_this->GetFieldsDescription(outFields);
		}
	}


	Camera::Camera()
		: Script()
	{
		ATN_WARN("TestProject.Camera::Instantiate");
	}

	void Camera::OnCreate()
	{
		ATN_WARN("TestProject.Camera::OnCreate");
	}

	void Camera::OnUpdate(Time frameTime)
	{
		GetComponent<TransformComponent>().Translation.x += m_Speed * frameTime.AsSeconds();
	}

	void Camera::GetFieldsDescription(ScriptFieldMap* outFields)
	{
		// ADD_FIELD(m_Speed);

		outFields->insert({ "m_Speed", ScriptFieldStorage(&m_Speed)});
		outFields->insert({ "m_Checkbox", ScriptFieldStorage(&m_Checkbox) });
	}
}
