#include "Mesh.h"

#include "Athena/Math/Vector.h"
#include "Athena/Renderer/Renderer.h"
#include "Color.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace Athena
{
	static void AssimpVector3ToAthenaVector3(const aiVector3D& input, Vector3& output)
	{
		output.x = input.x;
		output.y = input.y;
		output.z = input.z;
	}

	static Ref<VertexBuffer> AssimpMeshToVertexBuffer(aiMesh* aimesh)
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
		aiVector3D** texcoords = aimesh->mTextureCoords;
		std::vector<Vertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			AssimpVector3ToAthenaVector3(aimesh->mVertices[i], vertices[i].Position);

			// TexCoord
			if (aimesh->HasTextureCoords(0))
			{
				vertices[i].TexCoords.x = aimesh->mTextureCoords[0][i].x;
				vertices[i].TexCoords.y = aimesh->mTextureCoords[0][i].y;
			}

			// Normal
			if (aimesh->HasNormals())
			{
				AssimpVector3ToAthenaVector3(aimesh->mNormals[i], vertices[i].Normal);
			}
			else
			{
				vertices[i].Normal = Vector3(0);
			}
		}


		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(Vertex);
		vBufferDesc.pBufferLayout = &Renderer::GetVertexBufferLayout();
		vBufferDesc.pIndexBuffer = indexBuffer;
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}

	static void AssimpNodesToAthenaNodes(const aiScene* scene, aiNode* root, std::vector<MeshNode>& storage)
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
}
