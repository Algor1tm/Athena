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

	TransformComponent Entity::GetWorldTransform() const
	{
		if (HasComponent<ChildComponent>())
		{
			const TransformComponent& parentTransform = GetComponent<ChildComponent>().Parent.GetWorldTransform();
			const TransformComponent& currentTransform = GetComponent<TransformComponent>();

			TransformComponent combined;
			combined.Translation = parentTransform.Translation + currentTransform.Translation;
			combined.Rotation = parentTransform.Rotation * currentTransform.Rotation;
			combined.Scale = parentTransform.Scale * currentTransform.Scale;

			return combined;
		}

		return GetComponent<TransformComponent>();
	}
}
