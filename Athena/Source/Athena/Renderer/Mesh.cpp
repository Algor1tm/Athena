#include "Mesh.h"

#include "Athena/Math/Vector.h"
#include "Athena/Core/Color.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace Athena
{
	struct Vertex
	{
		Vector3 Position;
		LinearColor Color;
	};
	

	Ref<Mesh> Mesh::Create(const Filepath& filepath)
	{
		Ref<Mesh> result = CreateRef<Mesh>();
		result->m_Filepath = filepath;
		result->m_Name = filepath.stem().string();

		const aiScene* scene = aiImportFile(filepath.string().c_str(), aiProcessPreset_TargetRealtime_Fast);
		aiNode* root = scene->mRootNode;

		AssimpNodesToAthenaNodes(scene, root, result->m_Nodes);

		return result;
	}

	void Mesh::AssimpNodesToAthenaNodes(const aiScene* scene, aiNode* root, std::vector<MeshNode>& storage)
	{
		if (root != nullptr)
		{
			storage.reserve(root->mNumChildren);

			if (root->mNumMeshes > 0)
			{
				MeshNode node;
				node.Name = root->mName.C_Str();
				node.SubMeshes.resize(root->mNumMeshes);

				for (uint32 j = 0; j < root->mNumMeshes; ++j)
				{
					aiMesh* aimesh = scene->mMeshes[root->mMeshes[j]];
					node.SubMeshes[j] = AssimpMeshToVertexBuffer(aimesh);
				}

				storage.push_back(std::move(node));
			}

			for (uint32 i = 0; i < root->mNumChildren; ++i)
			{
				AssimpNodesToAthenaNodes(scene, root->mChildren[i], storage);
			}
		}
	}

	Ref<VertexBuffer> Mesh::AssimpMeshToVertexBuffer(aiMesh* aimesh)
	{
		uint32 numFaces = aimesh->mNumFaces;
		aiFace* faces = aimesh->mFaces;

		std::vector<uint32> indicies(numFaces * 3);

		uint32 index = 0;
		for (uint32 i = 0; i < numFaces; i++)
		{
			if (faces[i].mNumIndices != 3)
				break;

			indicies[index++] = faces[i].mIndices[0];
			indicies[index++] = faces[i].mIndices[1];
			indicies[index++] = faces[i].mIndices[2];
		}

		Ref<IndexBuffer> indexBuffer = IndexBuffer::Create(indicies.data(), indicies.size());

		uint32 numVertices = aimesh->mNumVertices;
		aiVector3D* positions = aimesh->mVertices;

		std::vector<Vertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			Vertex vertex;
			vertex.Position.x = positions[i].x;
			vertex.Position.y = positions[i].y;
			vertex.Position.z = positions[i].z;

			vertex.Color = LinearColor::Gray;

			vertices[i] = vertex;
		}

		BufferLayout layout =
		{
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float4, "a_Color" }
		};

		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(Vertex);
		vBufferDesc.pBufferLayout = &layout;
		vBufferDesc.pIndexBuffer = indexBuffer;
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}
}
