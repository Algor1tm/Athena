#include "Animation.h"

#include "Athena/Math/Transforms.h"
#include "Athena/Renderer/Mesh.h"

#include <stack>


namespace Athena
{
	static void BoneCreateHelper(const BoneStructureInfo& info, Bone& bone, uint32& size)
	{
		bone.Name = info.Name;
		bone.ID = size;

		bone.Children.resize(info.Children.size());
		for (uint32 i = 0; i < info.Children.size(); ++i)
		{
			size += 1;
			BoneCreateHelper(info.Children[i], bone.Children[i], size);
		}
	}

	Ref<Skeleton> Skeleton::Create(const BoneStructureInfo& boneHierarchy)
	{
		Ref<Skeleton> result = CreateRef<Skeleton>();

		uint32 boneCount = 0;
		BoneCreateHelper(boneHierarchy, result->m_RootBone, boneCount);
		boneCount++;

		result->m_BoneOffsetMatrices.reserve(boneCount);
		Matrix4 identity = Matrix4::Identity();

		for (uint32 i = 0; i < boneCount; ++i)
		{
			result->m_BoneOffsetMatrices[i] = identity;
		}

		return result;
	}

	void Skeleton::SetBoneOffsetMatrix(const String& name, const Matrix4& transform)
	{
		uint32 id = GetBoneID(name);
		m_BoneOffsetMatrices.at(id) = transform;
	}

	uint32 Skeleton::GetBoneID(const String& name) const
	{
		std::stack<const Bone*> boneStack;
		boneStack.push(&m_RootBone);

		while (!boneStack.empty())
		{
			const Bone* bone = boneStack.top();
			boneStack.pop();

			if (bone->Name == name)
				return bone->ID;

			for (uint32 i = 0; i < bone->Children.size(); ++i)
				boneStack.push(&bone->Children[i]);
		}

		ATN_CORE_ASSERT(false, "Invalid name for bone");
		return -1;
	}

	Ref<Animation> Animation::Create(const AnimationDescription& desc)
	{
		ATN_CORE_ASSERT(desc.BoneNameToKeyFramesMap.size() == desc.Skeleton->GetBoneCount(), "Invalid AnimationDescription!");

		Ref<Animation> result = CreateRef<Animation>();

		result->m_Name = desc.Name;
		result->m_Duration = desc.Duration;
		result->m_TicksPerSecond = desc.TicksPerSecond;
		result->m_Skeleton = desc.Skeleton;

		result->m_FinalTransforms.reserve(desc.BoneNameToKeyFramesMap.size());
		for (const auto& [name, keyframes] : desc.BoneNameToKeyFramesMap)
		{
			result->m_BoneIDToKeyFramesMap[desc.Skeleton->GetBoneID(name)] = keyframes;
			result->m_FinalTransforms.push_back(Matrix4::Identity());
		}

		return result;
	}

	void Animation::SetBonesState(float time)
	{
		const Bone& root = m_Skeleton->GetRootBone();
		SetBoneTransform(root, Matrix4::Identity(), time);
	}

	void Animation::Reset()
	{
		Matrix4 identity = Matrix4::Identity();
		for (uint32 i = 0; i < m_FinalTransforms.size(); ++i)
			m_FinalTransforms[i] = identity;
	}

	void Animation::SetBoneTransform(const Bone& bone, const Matrix4& parentTransform, float time)
	{
		Matrix4 boneTransform = GetInterpolatedLocalTransform(bone.ID, time);
		Matrix4 globalTransform = boneTransform * parentTransform;

		const Matrix4& boneOffset = m_Skeleton->GetBoneOffset(bone.ID);

		m_FinalTransforms[bone.ID] = boneOffset * globalTransform;

		for (uint32 i = 0; i < bone.Children.size(); ++i)
		{
			SetBoneTransform(bone.Children[i], globalTransform, time);
		}
	}

	Matrix4 Animation::GetInterpolatedLocalTransform(uint32 boneID, float time)
	{
		const std::vector<KeyFrame>& keyFrames = m_BoneIDToKeyFramesMap[boneID];

		if (keyFrames.size() == 1)
		{
			const KeyFrame& keyFrame = keyFrames[0];
			return Math::ToMat4(keyFrame.Rotation).Scale(keyFrame.Scale).Translate(keyFrame.Translation);
		}

		KeyFrame target;
		target.TimeStamp = time;
		auto iter = std::lower_bound(keyFrames.begin(), keyFrames.end(), target, 
			[](const KeyFrame& left, const KeyFrame& right) { return left.TimeStamp < right.TimeStamp; });
		
		iter--;
		ATN_CORE_ASSERT(iter >= keyFrames.begin() && iter <= (keyFrames.end() - 2));
		
		auto startIter = iter;
		auto endIter = iter + 1;
		
		const KeyFrame& start = *startIter;
		const KeyFrame& end = *endIter;

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);

		ATN_CORE_ASSERT(scaleFactor >= 0 && scaleFactor <= 1);

		Vector3 translation = Math::Lerp(start.Translation, end.Translation, scaleFactor);
		Quaternion rotation = Math::SLerp(start.Rotation, end.Rotation, scaleFactor);
		Vector3 scale = Math::Lerp(start.Scale, end.Scale, scaleFactor);

		return Math::ToMat4(rotation).Scale(scale).Translate(translation);
	}
	

	Ref<Animator> Animator::Create(const Ref<SkeletalMesh>& mesh)
	{
		Ref<Animator> result = CreateRef<Animator>();
		result->m_Mesh = mesh;

		return result;
	}

	void Animator::OnUpdate(Time frameTime)
	{
		if (IsPlaying())
		{
			m_CurrentTime += m_Animation->GetTicksPerSecond() * frameTime.AsSeconds();
			m_CurrentTime = Math::FMod(m_CurrentTime, m_Animation->GetDuration());

			m_Animation->SetBonesState(m_CurrentTime);
		}
		else
		{
			ATN_CORE_WARN("Animator::OnUpdate: Animation is unset!");
		}
	}

	void Animator::StopAnimation()
	{
		m_CurrentTime = 0.f;

		if (m_Animation != nullptr)
		{
			m_Animation->Reset();
			m_Animation = nullptr;
		}
	}

	void Animator::PlayAnimation(const Ref<Animation>& animation)
	{
		if (m_Mesh->HasAnimation(animation))
		{
			StopAnimation();
			m_Animation = animation;
		}
		else
		{
			ATN_CORE_WARN("Animator::PlayAnimaton: Attempt to play animation that does not belong to SkeletalMesh!");
		}
	}
}
