#include "Importer3D.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Material.h"

#include "Entity.h"

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

	static Ref<Material> AssimpMaterialToAthenaMaterial(aiMaterial* aimaterial, const Filepath& currentFilepath)
	{
		MaterialDescription desc;

		aiColor4D color;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_BASE_COLOR, color))
		{
			desc.Albedo = Vector3(color.r, color.g, color.b);
		}
		else if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
		{
			desc.Albedo = Vector3(color.r, color.g, color.b);
		}

		float roughness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			desc.Roughness = roughness;

		float metalness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
		{
			desc.Metalness = metalness;
		}

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_BASE_COLOR, 0), texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.AlbedoTexture = Texture2D::Create(path);
			desc.UseAlbedoTexture = true;
		}
		else if(AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.AlbedoTexture = Texture2D::Create(path);
			desc.UseAlbedoTexture = true;
		}

		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.NormalMap = Texture2D::Create(path);
			desc.UseNormalMap = true;
		}
		if (AI_SUCCESS == aimaterial->GetTexture(AI_MATKEY_ROUGHNESS_TEXTURE, &texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.RoughnessMap = Texture2D::Create(path);
			desc.UseRoughnessMap = true;
		}
		if (AI_SUCCESS == aimaterial->GetTexture(AI_MATKEY_METALLIC_TEXTURE, &texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.MetalnessMap = Texture2D::Create(path);
			desc.UseMetalnessMap = true;
		}
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_AMBIENT_OCCLUSION, 0), texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.AmbientOcclusionMap = Texture2D::Create(path);
			desc.UseAmbientOcclusionMap = true;
		}

		return MaterialManager::CreateMaterial(desc, aimaterial->GetName().C_Str());
	}

	static void LoadAssimpMesh(aiMesh* aimesh, Ref<VertexBuffer>& vertexBuffer, AABB& box)
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
			if (aimesh->HasPositions())
			{
				AssimpVector3ToAthenaVector3(aimesh->mVertices[i], vertices[i].Position);
				box.Extend(vertices[i].Position);
			}
			
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

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangents
				AssimpVector3ToAthenaVector3(aimesh->mTangents[i], vertices[i].Tangent);
				// Bitangent
				AssimpVector3ToAthenaVector3(aimesh->mBitangents[i], vertices[i].Bitangent);
			}

		}


		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(Vertex);
		vBufferDesc.pBufferLayout = &Renderer::GetVertexBufferLayout();
		vBufferDesc.pIndexBuffer = indexBuffer;
		vBufferDesc.Usage = BufferUsage::STATIC;

		vertexBuffer = VertexBuffer::Create(vBufferDesc);
	}

	static void LoadAssimpMeshesAsStaticMesh(const aiScene* scene, aiNode* root, std::vector<Ref<VertexBuffer>>& storage, AABB& box)
	{
		if (root != nullptr)
		{
			for (uint32 j = 0; j < root->mNumMeshes; ++j)
			{
				Ref<VertexBuffer> vertexBuffer;
				aiMesh* aimesh = scene->mMeshes[root->mMeshes[j]];
				LoadAssimpMesh(aimesh, vertexBuffer, box);

				storage.push_back(vertexBuffer);
			}

			for (uint32 i = 0; i < root->mNumChildren; ++i)
			{
				LoadAssimpMeshesAsStaticMesh(scene, root->mChildren[i], storage, box);
			}
		}
	}

	Importer3D::Importer3D(Ref<Scene> scene)
		: m_Context(scene)
	{

	}

	Importer3D::~Importer3D()
	{
		Release();
	}

	bool Importer3D::ImportScene(const Filepath& filepath)
	{
		const aiScene* aiscene = OpenFile(filepath);
		if (!aiscene)
			return false;

		if (aiscene->mNumMaterials < 2)
		{
			StaticMeshImportInfo info;

			Ref<StaticMesh> mesh = CreateRef<StaticMesh>();
			ImportStaticMesh(filepath, info, mesh);

			Entity entity = m_Context->CreateEntity();
			entity.GetComponent<TagComponent>().Tag = mesh->ImportInfo.Name;
			entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
		}
		else
		{
			ProcessNode(aiscene, aiscene->mRootNode);
		}

		return true;
	}

	bool Importer3D::ImportStaticMesh(const Filepath& filepath, const StaticMeshImportInfo& info, Ref<StaticMesh> outMesh)
	{
		const aiScene* aiscene = OpenFile(filepath);
		if (!aiscene)
			return false;

		if(outMesh == nullptr)
			outMesh = CreateRef<StaticMesh>();

		outMesh->Filepath = filepath;

		if (info.Indices.size() > 0)
		{
			outMesh->ImportInfo = info;
			outMesh->Vertices.resize(info.Indices.size());

			for (uint32 i = 0; i < info.Indices.size(); ++i)
				LoadAssimpMesh(aiscene->mMeshes[info.Indices[i]], outMesh->Vertices[i], outMesh->BoundingBox);

			Ref<Material> material = AssimpMaterialToAthenaMaterial(aiscene->mMaterials[info.MaterialIndex], m_CurrentFilepath);
			outMesh->MaterialName = material->GetName();
		}
		else
		{
			if (aiscene->mNumMaterials != 0)
			{
				if (aiscene->mNumMaterials > 1)
				{
					ATN_CORE_ERROR("Currently does not support StaticMesh with >= 2 materials!");
					return false;
				}
				else
				{
					Ref<Material> material = AssimpMaterialToAthenaMaterial(aiscene->mMaterials[0], m_CurrentFilepath);
					outMesh->MaterialName = material->GetName();
				}
			}

			outMesh->ImportInfo.Name = filepath.stem().string();
			outMesh->Vertices.clear();
			outMesh->Vertices.reserve(aiscene->mNumMeshes);
			LoadAssimpMeshesAsStaticMesh(aiscene, aiscene->mRootNode, outMesh->Vertices, outMesh->BoundingBox);
		}

		return true;
	}

	void Importer3D::ProcessNode(const aiScene* aiscene, aiNode* node)
	{
		for (uint32 i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* aimesh = aiscene->mMeshes[node->mMeshes[i]];

			Ref<StaticMesh> mesh = CreateRef<StaticMesh>();

			StaticMeshImportInfo info;
			info.Name = node->mName.C_Str();
			info.Indices = { node->mMeshes[i] };
			info.MaterialIndex = aimesh->mMaterialIndex;
			mesh->ImportInfo = info;

			mesh->Filepath = m_CurrentFilepath;
			Ref<VertexBuffer> buffer;
			LoadAssimpMesh(aimesh, buffer, mesh->BoundingBox);
			mesh->Vertices.push_back(buffer);
			
			Ref<Material> material = AssimpMaterialToAthenaMaterial(aiscene->mMaterials[info.MaterialIndex], m_CurrentFilepath);
			mesh->MaterialName = material->GetName();

			Entity entity = m_Context->CreateEntity();
			entity.GetComponent<TagComponent>().Tag = mesh->ImportInfo.Name;
			entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
		}

		for (uint32 i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNode(aiscene, node->mChildren[i]);
		}
	}

	const aiScene* Importer3D::OpenFile(const Filepath& filepath)
	{
		if (m_ImportedScenes.find(filepath) == m_ImportedScenes.end())
		{
			unsigned int flags = aiProcessPreset_TargetRealtime_Fast |
				aiProcess_PreTransformVertices |
				aiProcess_RemoveRedundantMaterials |
				aiProcess_OptimizeMeshes;

			if (RendererAPI::GetAPI() == RendererAPI::Direct3D)
				flags |= aiProcess_MakeLeftHanded | aiProcess_FlipUVs;

			const aiScene* aiscene = aiImportFile(filepath.string().c_str(), flags);

			if (aiscene == nullptr || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE)
			{
				aiReleaseImport(aiscene);
				const char* error = aiGetErrorString();
				ATN_CORE_ERROR("Importer3D Error: {0}", error);
				return nullptr;
			}

			m_ImportedScenes[filepath] = aiscene;
		}

		m_CurrentFilepath = filepath;

		return m_ImportedScenes[filepath];
	}

	void Importer3D::Release()
	{
		for (auto& [_, aiscene] : m_ImportedScenes)
		{
			aiReleaseImport(aiscene);
		}
	}
}
