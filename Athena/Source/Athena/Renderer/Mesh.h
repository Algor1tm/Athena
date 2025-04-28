#pragma once

#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/Core.h"
#include "Athena/Renderer/AABB.h"
#include "Athena/Renderer/GPUBuffer.h"
#include "Athena/Renderer/Animation.h"
#include "Athena/Renderer/MaterialAsset.h"
#include "Athena/Renderer/Renderer.h"

#include <vector>


class aiScene;
class aiNode;


namespace Athena
{
	struct StaticVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;

		static VertexMemoryLayout GetLayout()
		{
			return {
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoords" },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" } };
		}
	};

	struct AnimVertex
	{
		Vector3 Position;
		Vector2 TexCoords;
		Vector3 Normal;
		Vector3 Tangent;
		Vector3 Bitangent;
		int BoneIDs[ShaderDef::MAX_NUM_BONES_PER_VERTEX];
		float Weights[ShaderDef::MAX_NUM_BONES_PER_VERTEX];

		static VertexMemoryLayout GetLayout()
		{
			return {
				{ ShaderDataType::Float3, "a_Position"  },
				{ ShaderDataType::Float2, "a_TexCoords" },
				{ ShaderDataType::Float3, "a_Normal"    },
				{ ShaderDataType::Float3, "a_Tangent"   },
				{ ShaderDataType::Float3, "a_Bitangent" },
				{ ShaderDataType::Int4,   "a_Tangent"   },
				{ ShaderDataType::Float4, "a_Bitangent" } };
		}
	};

#if 0
	struct SubMesh
	{
		String Name;
		String MaterialName;
		Ref<VertexBuffer> VertexBuffer;
	};

	using MaterialTable = std::unordered_map<String, AssetHandle>;

	class ATHENA_API StaticMesh
	{
	public:
		static Ref<StaticMesh> Create(const FilePath& path);

		const std::vector<SubMesh>& GetAllSubMeshes() const { return m_SubMeshes; }

		const String& GetName() const { return m_Name; }
		const FilePath& GetFilePath() const { return m_FilePath; }
		const AABB& GetBoundingBox() const { return m_AABB; }
		MaterialTable& GetMaterialTable() { return m_MaterialTable; }
		
		const Ref<Animator>& GetAnimator() { return m_Animator; }

		bool HasAnimations() const { return m_Animator != nullptr; }

	private:
		void TraverseNodes(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform);

	private:
		FilePath m_FilePath;
		String m_Name;
		AABB m_AABB;
		std::vector<SubMesh> m_SubMeshes;
		MaterialTable m_MaterialTable;

		Ref<Skeleton> m_Skeleton;
		Ref<Animator> m_Animator;
	};
#else

	struct SubMesh
	{
		String Name;
		String MaterialName;
		Ref<VertexBuffer> VertexBuffer;
		std::vector<uint32> Children;
		AABB AABB;
	};

	using MaterialTable = std::unordered_map<String, AssetHandle>;

	class ATHENA_API MeshSource: public Asset
	{
	public:
		static Ref<MeshSource> Create();

		virtual AssetType GetAssetType() const override { return AssetType::MeshSource; }

		bool HasSubMeshes() const { return !m_SubMeshes.empty(); }
		const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		const SubMesh& GetSubMesh(uint32 index) const { return m_SubMeshes[index]; }
		const SubMesh& GetRootSubMesh() const { return GetSubMesh(0); }

		MaterialTable& GetMaterialTable() { return m_MaterialTable; }
		const AABB& GetBoundingBox() const { return m_AABB; }

	private:
		std::vector<SubMesh> m_SubMeshes;
		MaterialTable m_MaterialTable;
		AABB m_AABB;

		friend class AssimpImporter;
	};


	class ATHENA_API StaticMesh: public Asset
	{
	public:
		static Ref<StaticMesh> Create();
		static Ref<StaticMesh> Create(AssetHandle meshSourceHandle);

		virtual AssetType GetAssetType() const override { return AssetType::StaticMesh; }

		AssetHandle GetMeshSource() const { return m_MeshSource; }
		MaterialTable& GetMaterialTable() { return m_MaterialTable; }
		const std::vector<uint32>& GetSubMeshIndices() const { return m_SubMeshIndices; }

	private:
		AssetHandle m_MeshSource;
		MaterialTable m_MaterialTable;
		std::vector<uint32> m_SubMeshIndices;

		friend class StaticMeshSerializer;
	};

#endif
}
