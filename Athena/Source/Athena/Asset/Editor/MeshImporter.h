#pragma once

#include "Athena/Asset/Asset.h"
#include "Athena/Core/Core.h"
#include "Athena/Renderer/Mesh.h"
#include "Athena/Renderer/Animation.h"


class aiScene;
class aiNode;
class aiMesh;
class aiMaterial;


namespace Athena
{
	class ATHENA_API MeshImportSettings : public AssetImportSettings
	{
	public:
		virtual Ref<AssetImportSettings> Clone() const override;

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath) override;

		bool ImportAnimations = true; // Cannot import animations when not collapsing graph
		bool CollapseGraph = true;
		float Scale = 1.f;
		std::vector<uint32> SubMeshIndices;
		std::unordered_map<String, AssetHandle> OverrideMaterials;
	};


	class MeshImporter
	{
	public:
		MeshImporter(const Ref<MeshImportSettings>& settings);

		bool ImportToMesh(const FilePath& path, WeakRef<Mesh> mesh);

		Ref<Animation> ImportAnimation(uint32 animationIndex, const Ref<Skeleton>& skeleton) const;
		Ref<Skeleton> ImportSkeleton() const;
		bool HasSkeleton() const;

	private:
		const aiNode* FindRootNode(const aiNode* root);
		void TraverseNodes(const Ref<Mesh>& mesh, const aiNode* ainode, const Matrix4& parentTransform = Matrix4::Identity()) const;
		void BuildBonesHierarchy(const aiNode* ainode, const std::unordered_map<String, Matrix4>& bonesMap, const Matrix4& parentTransform, std::vector<Bone>& bones) const;

		void LoadGeometry(const Ref<Mesh>& mesh, const std::vector<uint32>& subMeshIndices) const;
		AssetHandle LoadMaterial(const aiMaterial* aimaterial) const;
		AssetHandle LoadMaterialTexture(const aiMaterial* aimaterial, uint32 type, bool srgb) const;

	private:
		Ref<MeshImportSettings> m_Settings;
		FilePath m_Path;
		const aiScene* m_aiScene;
	};
}
