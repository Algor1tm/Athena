#include "ScriptEntity.h"
#include "ScriptEngine.h"


namespace Athena
{
	Vector3& ScriptEntity::GetTranslation()
	{
		//Scene* scene = ScriptEngine::GetSceneContext();
		//
		//Entity proxy = Entity{ m_ID, scene };
		//return proxy.GetComponent<TransformComponent>().Translation;
		return Vector3::Forward();
	}
}
