#include "Animation.h"

#include "Athena/Math/Transforms.h"

#include "Athena/Renderer/Mesh.h"

#include <stack>


namespace Athena
{
	Ref<Skeleton> Skeleton::Create(const std::vector<Bone>& bones)
	{
		Ref<Skeleton> result = Ref<Skeleton>::Create();
		result->m_Bones = bones;
		return result;
	}

	void Skeleton::SetBoneOffsetMatrix(uint32 index, const Matrix4& transform)
	{
		m_Bones[index].OffsetMatrix = transform;
	}

	uint32 Skeleton::GetBoneIndex(const String& name) const
	{
		for (const auto& bone : m_Bones)
		{
			if (bone.Name == name)
				return bone.Index;
		}

		ATN_CORE_ASSERT(false);
		return 0;
	}

	Ref<Animation> Animation::Create(const AnimationCreateInfo& info)
	{
		ATN_CORE_VERIFY(info.BoneNameToKeyFramesMap.size() == info.Skeleton->GetBoneCount());

		Ref<Animation> result = Ref<Animation>::Create();

		result->m_Name = info.Name;
		result->m_Duration = info.Duration;
		result->m_TicksPerSecond = info.TicksPerSecond;
		result->m_Skeleton = info.Skeleton;

		for (const auto& [name, keyframes] : info.BoneNameToKeyFramesMap)
		{
			result->m_BoneIndexToKeyFramesMap[info.Skeleton->GetBoneIndex(name)] = keyframes;
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
		Matrix4 boneTransform = GetInterpolatedLocalTransform(bone.Index, time);
		Matrix4 globalTransform = boneTransform * parentTransform;

		const Matrix4& boneOffset = bone.OffsetMatrix;

		transforms[bone.Index] = boneOffset * globalTransform;

		for (uint32 i = 0; i < bone.Children.size(); ++i)
		{
			ProcessBonesHierarchy(m_Skeleton->GetBone(bone.Children[i]), globalTransform, time, transforms);
		}
	}

	Matrix4 Animation::GetInterpolatedLocalTransform(uint32 boneIndex, float time)
	{
		const KeyFramesList& keyFrames = m_BoneIndexToKeyFramesMap[boneIndex];

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

		const TranslationKey& start = *iter;
		const TranslationKey& end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);

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

		RotationKey start = *iter;
		RotationKey end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);

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

		const ScaleKey& start = *iter;
		const ScaleKey& end = *(iter + 1);

		float scaleFactor = (time - start.TimeStamp) / (end.TimeStamp - start.TimeStamp);

		return Math::Lerp(start.Value, end.Value, scaleFactor);
	}


	Ref<Animator> Animator::Create(const std::vector<Ref<Animation>>& animations, const Ref<Skeleton>& skeleton)
	{
		Ref<Animator> result = Ref<Animator>::Create();

		result->m_Skeleton = skeleton;
		result->m_Animations = animations;
		result->m_CurrentTime = 0.f;

		result->m_BoneTransforms.resize(skeleton->GetBoneCount());
		result->StopAnimation();

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

		Matrix4 identity = Matrix4::Identity();
		for (uint32 i = 0; i < m_BoneTransforms.size(); ++i)
			m_BoneTransforms[i] = identity;
	}

	void Animator::PlayAnimation(const Ref<Animation>& animation)
	{
		m_CurrentTime = 0.f;
		m_CurrentAnimation = nullptr;

		if (std::find(m_Animations.begin(), m_Animations.end(), animation) != m_Animations.end())
		{
			m_CurrentAnimation = animation;
		}
		else
		{
			ATN_CORE_WARN_TAG("Animator", "Attempt to play Animation that does not belong to Animator!");
		}
	}
}
