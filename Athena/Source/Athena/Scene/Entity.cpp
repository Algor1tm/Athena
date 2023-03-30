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

	bool Entity::HasChildren() const
	{
		return GetComponent<ParentComponent>().Children.size() > 0;
	}

	Matrix4 Entity::GetWorldTransform() const
	{
		if (HasComponent<ChildComponent>())
		{
			Entity parent = GetComponent<ChildComponent>().Parent;
			Matrix4 parentTransform = parent.GetWorldTransform();
			return parentTransform * GetComponent<TransformComponent>().AsMatrix();
		}

		return GetComponent<TransformComponent>().AsMatrix();
	}
}
