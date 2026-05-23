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
	struct MeshVertex
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

	struct BoneInfluenceVertex
	{
		int BoneIDs[ShaderDef::MAX_NUM_BONES_PER_VERTEX];
		float Weights[ShaderDef::MAX_NUM_BONES_PER_VERTEX];

		static VertexMemoryLayout GetLayout()
		{
			return {
				{ ShaderDataType::Int4,   "a_BoneIDs"   },
				{ ShaderDataType::Float4, "a_Weights"   } };
		}
	};

	struct SubMesh
	{
		uint32 BaseVertex;
		uint32 VertexCount;
		uint32 BaseIndex;
		uint32 IndexCount;

		String Name;
		String NodeName;
		String MaterialName;
		AABB AABB;

		Matrix4 Transform = Matrix4::Identity();
		Matrix4 LocalTransform = Matrix4::Identity();
	};


	struct MeshNode
	{
		String Name;
		uint32 Index = 0xffffffff;
		uint32 Parent = 0xffffffff;
		std::vector<uint32> Children;
		std::vector<uint32> SubMeshes;

		Matrix4 LocalTransform = Matrix4::Identity();

		bool IsRoot() const { return Parent == 0xffffffff; }
	};


	using MaterialTable = std::unordered_map<String, AssetHandle>;


	class ATHENA_API Mesh : public Asset
	{
	public:
		Mesh();

		virtual AssetType GetAssetType() const override { return AssetType::Mesh; }

		virtual bool Serialize(const FilePath& absolutePath) const override;
		virtual bool Deserialize(const FilePath& absolutePath, Ref<AssetImportSettings> importSettings) override;

		Ref<VertexBuffer> GetVertexBuffer() const { return m_VertexBuffer; }
		Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		Ref<VertexBuffer> GetBonesInfluenceBuffer() const { return m_BonesInfluenceBuffer; }

		const std::vector<MeshNode>& GetMeshNodes() const { return m_Nodes; }
		bool HasMeshNode(uint32 index) const { return index < m_Nodes.size(); }
		const MeshNode& GetMeshNode(uint32 index) const { return m_Nodes[index]; }
		const MeshNode& GetRootNode() const { return GetMeshNode(0); }

		bool HasSubMeshes() const { return !m_SubMeshes.empty(); }
		const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		bool HasSubMesh(uint32 index) const { return index < m_SubMeshes.size(); }
		const SubMesh& GetSubMesh(uint32 index) const { return m_SubMeshes[index]; }

		const AABB& GetBoundingBox() const { return m_AABB; }

		Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }
		const std::vector<Ref<Animation>> GetAnimations() const { return m_Animations; }
		bool HasAnimation(const Ref<Animation>& animation) const;
		bool IsRigged() const { return m_IsRigged; }

		MaterialTable& GetMaterialTable() { return m_MaterialTable; }
		Ref<MaterialAsset> GetMaterial(const String& materialName) const;

		bool IsCollapsedGraph() const { return m_CollapsedGraph; }

	private:
		Ref<VertexBuffer> m_VertexBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		Ref<VertexBuffer> m_BonesInfluenceBuffer;

		std::vector<MeshNode> m_Nodes;
		std::vector<SubMesh> m_SubMeshes;
		MaterialTable m_MaterialTable;
		
		Ref<Skeleton> m_Skeleton;
		std::vector<Ref<Animation>> m_Animations;
		bool m_IsRigged = false;

		bool m_CollapsedGraph = true;
		AABB m_AABB;

		friend class MeshImporter;
	};
}
