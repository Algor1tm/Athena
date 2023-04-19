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
			const TransformComponent& localTransform = GetComponent<TransformComponent>();

			TransformComponent worldTransform;

			worldTransform.Translation = parentTransform.Translation + parentTransform.Rotation * localTransform.Translation;
			worldTransform.Rotation = parentTransform.Rotation * localTransform.Rotation;
			worldTransform.Scale = parentTransform.Scale * localTransform.Scale;

			return worldTransform;
		}

		return GetComponent<TransformComponent>();
	}
}