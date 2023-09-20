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

	TransformComponent Entity::GetWorldTransform() const
	{
		if (HasComponent<ParentComponent>())
		{
			const TransformComponent& parentTransform = GetComponent<ParentComponent>().Parent.GetWorldTransform();
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
