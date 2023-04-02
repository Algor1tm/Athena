#include "Animation.h"

#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Mesh.h"

#include <stack>


namespace Athena
{
	static void BoneCreateHelper(const BonesHierarchyInfo& info, Bone& bone, uint32& size)
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

	Ref<Skeleton> Skeleton::Create(const BonesHierarchyInfo& boneHierarchy)
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

		for (const auto& [name, keyframes] : desc.BoneNameToKeyFramesMap)
		{
			result->m_BoneIDToKeyFramesMap[desc.Skeleton->GetBoneID(name)] = keyframes;
		}

		return result;
	}

	void Animation::GetBoneTransforms(float time, std::vector<Matrix4>& transforms)
	{
		const Bone& root = m_Skeleton->GetRootBone();
		ProcessBonesHierarchy(root, Matrix4::Identity(), time, transforms);
	}

	void Animation::ProcessBonesHierarchy(const Bone& bone, const Matrix4& parentTransform, float time, std::vector<Matrix4>& transforms)
	{
		Matrix4 boneTransform = GetInterpolatedLocalTransform(bone.ID, time);
		Matrix4 globalTransform = boneTransform * parentTransform;

		const Matrix4& boneOffset = m_Skeleton->GetBoneOffset(bone.ID);

		transforms[bone.ID] = boneOffset * globalTransform;

		for (uint32 i = 0; i < bone.Children.size(); ++i)
		{
			ProcessBonesHierarchy(bone.Children[i], globalTransform, time, transforms);
		}
	}

	Matrix4 Animation::GetInterpolatedLocalTransform(uint32 boneID, float time)
	{
		const KeyFramesList& keyFrames = m_BoneIDToKeyFramesMap[boneID];

		Vector3 translation = GetInterpolatedTranslation(keyFrames.TranslationKeys, time);
		Quaternion rotation = GetInterpolatedRotation(keyFrames.RotationKeys, time);
		Vector3 scale = GetInterpolatedScale(keyFrames.ScaleKeys, time);

		return Math::ConstructTransform(translation, scale, rotation);
	}
	
	Vector3 Animation::GetInterpolatedTranslation(const std::vector<TranslationKey>& keys, float time)
	{
		if (keys.size() == 1)
			return keys[0].Value;

		TranslationKey target;
		target.TimeStamp = time;
		auto iter = std::lower_bound(keys.begin(), keys.end(), target,
			[](const TranslationKey& left, const TranslationKey& right) { return left.TimeStamp <= right.TimeStamp; });

		iter--;
		ATN_CORE_ASSERT(iter >= keys.begin() && iter <= (keys.end() - 2));

		const TranslationKey& start = *iter;
		const TranslationKey& end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);
		ATN_CORE_ASSERT(scaleFactor >= 0 && scaleFactor <= 1);

		return Math::Lerp(start.Value, end.Value, scaleFactor);
	}

	Quaternion Animation::GetInterpolatedRotation(const std::vector<RotationKey>& keys, float time)
	{
		if (keys.size() == 1)
			return keys[0].Value;

		RotationKey target;
		target.TimeStamp = time;
		auto iter = std::lower_bound(keys.begin(), keys.end(), target,
			[](const RotationKey& left, const RotationKey& right) { return left.TimeStamp <= right.TimeStamp; });

		iter--;
		ATN_CORE_ASSERT(iter >= keys.begin() && iter <= (keys.end() - 2));

		const RotationKey& start = *iter;
		const RotationKey& end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);
		ATN_CORE_ASSERT(scaleFactor >= 0 && scaleFactor <= 1);

		return Math::SLerp(start.Value, end.Value, scaleFactor);
	}

	Vector3 Animation::GetInterpolatedScale(const std::vector<ScaleKey>& keys, float time)
	{
		if (keys.size() == 1)
			return keys[0].Value;

		ScaleKey target;
		target.TimeStamp = time;
		auto iter = std::lower_bound(keys.begin(), keys.end(), target,
			[](const ScaleKey& left, const ScaleKey& right) { return left.TimeStamp <= right.TimeStamp; });

		iter--;
		ATN_CORE_ASSERT(iter >= keys.begin() && iter <= (keys.end() - 2));

		const ScaleKey& start = *iter;
		const ScaleKey& end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);
		ATN_CORE_ASSERT(scaleFactor >= 0 && scaleFactor <= 1);

		return Math::Lerp(start.Value, end.Value, scaleFactor);
	}


	Ref<Animator> Animator::Create(const std::vector<Ref<Animation>>& animations)
	{
		Ref<Animator> result = CreateRef<Animator>();

		result->m_Animations = animations;
		result->m_CurrentTime = 0.f;

		return result;
	}

	void Animator::OnUpdate(Time frameTime)
	{
		if (IsPlaying())
		{
			m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * frameTime.AsSeconds();
			m_CurrentTime = Math::FMod(m_CurrentTime, m_CurrentAnimation->GetDuration());

			m_CurrentAnimation->GetBoneTransforms(m_CurrentTime, m_BoneTransforms);
		}
	}

	void Animator::StopAnimation()
	{
		m_CurrentTime = 0.f;
		m_CurrentAnimation = nullptr;
	}

	void Animator::PlayAnimation(const Ref<Animation>& animation)
	{
		StopAnimation();

		if (std::find(m_Animations.begin(), m_Animations.end(), animation) != m_Animations.end())
		{
			m_CurrentAnimation = animation;

			m_BoneTransforms.resize(m_CurrentAnimation->GetSkeleton()->GetBoneCount());
			Matrix4 identity = Matrix4::Identity();
			for (uint32 i = 0; i < m_BoneTransforms.size(); ++i)
				m_BoneTransforms[i] = identity;
		}
		else
		{
			ATN_CORE_WARN("Animator::PlayAnimation: Attempt to play Animation that does not belong to Animator!");
		}
	}
}
