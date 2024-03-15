#include "Athena/Scene/Entity.h"

#include "Athena/Scene/Components.h"


namespace Athena
{
	UUID Entity::GetID() const
	{
		return GetComponent<IDComponent>().ID; 
	}

	const String& Entity::GetName() const 
	{
		return GetComponent<TagComponent>().Tag; 
	}
}
