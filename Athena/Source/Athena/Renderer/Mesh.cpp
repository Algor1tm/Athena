#include "Mesh.h"

#include "Athena/Math/Vector.h"
#include "Athena/Scene/Scene.h"

#include "Color.h"
#include "Renderer.h"

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

	static Ref<Material> AssimpMaterialToAthenaMaterial(aiMaterial* aimaterial)
	{
		MaterialDescription desc;

		desc.Name = aimaterial->GetName().C_Str();

		bool metalnessWorkflow = true;
		aiColor4D color;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_BASE_COLOR, color))
		{
			desc.Albedo = Vector3(color.r, color.g, color.b);
		}
		else if(AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
		{
			ATN_CORE_WARN("Athena currently does not support specular PBR workflow!");
			metalnessWorkflow = false;
			desc.Albedo = Vector3(color.r, color.g, color.b);
		}

		float roughness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			desc.Roughness = roughness;

		float metalness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
			desc.Metalness = metalness;

		aiString texFilepath;
		if (aimaterial->Get(AI_MATKEY_TEXTURE(metalnessWorkflow ? aiTextureType_BASE_COLOR : aiTextureType_DIFFUSE, 0), texFilepath) == AI_SUCCESS)
		{
			desc.AlbedoTexture = Texture2D::Create(texFilepath.C_Str());
			desc.UseAlbedoTexture = true;
		}

		if (aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texFilepath) == AI_SUCCESS)
		{
			desc.NormalMap = Texture2D::Create(texFilepath.C_Str());
			desc.UseNormalMap = true;
		}

		if (aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), texFilepath) == AI_SUCCESS)
		{
			desc.RoughnessMap = Texture2D::Create(texFilepath.C_Str());
			desc.UseRoughnessMap = true;
		}

		if (aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), texFilepath) == AI_SUCCESS)
		{
			desc.MetalnessMap = Texture2D::Create(texFilepath.C_Str());
			desc.UseMetalnessMap = true;
		}

		return Material::Create(desc);
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

	static void LoadAssimpMeshes(const aiScene* scene, aiNode* root, std::vector<Ref<VertexBuffer>>& storage)
	{
		if (root != nullptr)
		{
			for (uint32 j = 0; j < root->mNumMeshes; ++j)
			{
				Ref<VertexBuffer> vertexBuffer;
				aiMesh* aimesh = scene->mMeshes[root->mMeshes[j]];
				vertexBuffer = AssimpMeshToVertexBuffer(aimesh);

				storage.push_back(vertexBuffer);
			}

			for (uint32 i = 0; i < root->mNumChildren; ++i)
			{
				LoadAssimpMeshes(scene, root->mChildren[i], storage);
			}
		}
	}

	Ref<StaticMesh> StaticMesh::Create(const Filepath& filepath, const Ref<Scene>& scene)
	{
		Ref<StaticMesh> result = CreateRef<StaticMesh>();
		result->m_Filepath = filepath;
		result->m_Name = filepath.stem().string();

		unsigned int flags = aiProcessPreset_TargetRealtime_Fast |
			aiProcess_PreTransformVertices |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_OptimizeMeshes;

		if (RendererAPI::GetAPI() == RendererAPI::Direct3D)
			flags |= aiProcess_MakeLeftHanded | aiProcess_FlipUVs;

		const aiScene* aiscene = aiImportFile(filepath.string().c_str(), flags);

		if (scene == nullptr || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE)
		{
			aiReleaseImport(aiscene);
			const char* error = aiGetErrorString();
			ATN_CORE_ERROR(error);
			return nullptr;
		}

		aiNode* root = aiscene->mRootNode;

		result->m_Vertices.reserve(aiscene->mNumMeshes);
		LoadAssimpMeshes(aiscene, root, result->m_Vertices);

		if (aiscene->mNumMaterials != 0)
		{
			if (aiscene->mNumMaterials > 1)
			{
				ATN_CORE_ERROR("Currently does not support StaticMesh with >= 2 materials!");
			}
			else 
			{
				Ref<Material> material = AssimpMaterialToAthenaMaterial(aiscene->mMaterials[0]);
				result->m_MaterialIndex = scene->AddMaterial(material);
			}
		}

		aiReleaseImport(aiscene);
		return result;
	}
}
