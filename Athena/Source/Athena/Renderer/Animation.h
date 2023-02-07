#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Time.h"

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"
#include "Athena/Math/Quaternion.h"


namespace Athena
{
	class ATHENA_API SkeletalMesh;

#define MAX_NUM_BONES_PER_VERTEX 4
#define MAX_NUM_BONES 512

	struct BoneStructureInfo
	{
		String Name;
		std::vector<BoneStructureInfo> Children;
	};

	struct Bone
	{
		String Name;
		uint32 ID;

		std::vector<Bone> Children;
	};

	class ATHENA_API Skeleton
	{
	public:
		static Ref<Skeleton> Create(const BoneStructureInfo& boneHierarchy);

		void SetBoneOffsetMatrix(const String& name, const Matrix4& transform);
		const Matrix4& GetBoneOffset(uint32 id) const { return m_BoneOffsetMatrices.at(id); }

		uint32 GetBoneID(const String& name) const;
		uint32 GetBoneCount() const { return m_BoneOffsetMatrices.size(); }

		const Bone& GetRootBone() const { return m_RootBone; }

	private:
		Bone m_RootBone;
		std::unordered_map<uint32, Matrix4> m_BoneOffsetMatrices;
	};


	struct KeyFrame
	{
		float TimeStamp;
		Vector3 Translation;
		Quaternion Rotation;
		Vector3 Scale;
	};

	struct AnimationDescription
	{
		String Name;
		float Duration = 0;
		uint32 TicksPerSecond = 30;
		std::unordered_map<String, std::vector<KeyFrame>> BoneNameToKeyFramesMap;
		Ref<Skeleton> Skeleton;
	};

	class ATHENA_API Animation
	{
	public:
		static Ref<Animation> Create(const AnimationDescription& desc);

		const std::vector<Matrix4>& GetBoneTransforms() const { return m_FinalTransforms; };

		void SetBonesState(float time);

		float GetDuration() const { return m_Duration; }
		uint32 GetTicksPerSecond() const { return m_TicksPerSecond; }

		const String& GetName() const { return m_Name; };

		void Reset();

	private:
		void SetBoneTransform(const Bone& bone, const Matrix4& parentTransform, float time);
		Matrix4 GetInterpolatedLocalTransform(uint32 boneID, float time);

	private:
		String m_Name;
		float m_Duration;
		uint32 m_TicksPerSecond;
		std::unordered_map<uint32, std::vector<KeyFrame>> m_BoneIDToKeyFramesMap;
		std::vector<Matrix4> m_FinalTransforms;
		Ref<Skeleton> m_Skeleton;
	};


	class ATHENA_API Animator
	{
	public:
		static Ref<Animator> Create(const Ref<SkeletalMesh>& mesh);

		void OnUpdate(Time frameTime);
		bool IsPlaying() const { return m_Animation != nullptr; }

		void StopAnimation();
		void PlayAnimation(const Ref<Animation>& animation);

		const Ref<Animation>& GetAnimation() const { return m_Animation; }

		float GetAnimationTime() const { return m_CurrentTime; }
		void SetAnimationTime(float time) { m_CurrentTime = time; }

	private:
		Ref<SkeletalMesh> m_Mesh;
		Ref<Animation> m_Animation;
		float m_CurrentTime;
	};
}
