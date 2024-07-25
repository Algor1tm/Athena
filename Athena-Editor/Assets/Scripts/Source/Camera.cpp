#include "Camera.h"


namespace TestProject
{
	extern "C"
	{
		SCRIPT_API void _Camera_Instantiate(::Athena::Script** outScript)
		{
			*outScript = new Camera();
		}

		SCRIPT_API void _Camera_OnCreate(::Athena::Script* script)
		{
			script->OnCreate();
		}

		SCRIPT_API void _Camera_OnUpdate(::Athena::Script* script, float frameTime)
		{
			script->OnUpdate(::Athena::Time::Milliseconds(frameTime));
		}
	}


	Camera::Camera()
		: Script()
	{
		ATN_CORE_ERROR("TestProject.Camera::Instantiate");
	}

	void Camera::OnCreate()
	{
		ATN_CORE_ERROR("TestProject.Camera::OnCreate");
	}

	void Camera::OnUpdate(Time frameTime)
	{
		GetComponent<TransformComponent>().Translation.x += 1 * frameTime.AsSeconds();
	}
}
