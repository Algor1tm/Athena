#include "Importer3D.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Material.h"

#include "Entity.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


namespace Athena
{
	static Vector3 aiVector3DToVector3(const aiVector3D& input)
	{
		return { input.x, input.y, input.z };
	}

	static void CopyaiVector3DToVector3(const aiVector3D& input, Vector3& output)
	{
		output.x = input.x;
		output.y = input.y;
		output.z = input.z;
	}

	static Ref<Material> ParseMaterial(aiMaterial* aimaterial, const Filepath& currentFilepath)
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
			desc.AlbedoMap = Texture2D::Create(path);
			desc.UseAlbedoMap = true;
		}
		else if(AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texFilepath))
		{
			Filepath path = currentFilepath;
			path.replace_filename(texFilepath.C_Str());
			desc.AlbedoMap = Texture2D::Create(path);
			desc.UseAlbedoMap = true;
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

	static Ref<StaticMesh> ParseStaticMesh(const aiScene* scene, uint32 aiMeshIndex, const Filepath& currentFilepath)
	{
		aiMesh* aimesh = scene->mMeshes[aiMeshIndex];

		Ref<StaticMesh> result = CreateRef<StaticMesh>();

		bool createBoundingBox = true;

		if (aiVector3DToVector3(aimesh->mAABB.mMin) != Vector3(0) || aiVector3DToVector3(aimesh->mAABB.mMax) != Vector3(0))
		{
			result->BoundingBox = AABB(aiVector3DToVector3(aimesh->mAABB.mMin), aiVector3DToVector3(aimesh->mAABB.mMax));
			createBoundingBox = false;
		}

		result->Name = aimesh->mName.C_Str();
		result->MaterialName = ParseMaterial(scene->mMaterials[aimesh->mMaterialIndex], currentFilepath)->GetName();
		result->Filepath = currentFilepath;
		result->aiMeshIndex = aiMeshIndex;

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
				CopyaiVector3DToVector3(aimesh->mVertices[i], vertices[i].Position);
				if (createBoundingBox)
					result->BoundingBox.Extend(vertices[i].Position);
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
				CopyaiVector3DToVector3(aimesh->mNormals[i], vertices[i].Normal);
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				CopyaiVector3DToVector3(aimesh->mTangents[i], vertices[i].Tangent);
				// Bitangent
				CopyaiVector3DToVector3(aimesh->mBitangents[i], vertices[i].Bitangent);
			}
		}

		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(Vertex);
		vBufferDesc.pBufferLayout = &Renderer::GetVertexBufferLayout();
		vBufferDesc.pIndexBuffer = indexBuffer;
		vBufferDesc.Usage = BufferUsage::STATIC;

		result->Vertices = VertexBuffer::Create(vBufferDesc);
		return result;
	}

	Importer3D::Importer3D(Ref<Scene> scene)
		: m_Scene(scene)
	{

	}

	Importer3D::~Importer3D()
	{
		Release();
	}

	bool Importer3D::Import(const Filepath& filepath)
	{
		const aiScene* aiscene = OpenFile(filepath);
		if (!aiscene)
			return false;

		for (uint32 i = 0; i < aiscene->mNumMeshes; ++i)
		{
			Ref<StaticMesh> mesh = ParseStaticMesh(aiscene, i, m_CurrentFilepath);

			Entity entity = m_Scene->CreateEntity();
			entity.GetComponent<TagComponent>().Tag = mesh->Name;
			entity.AddComponent<StaticMeshComponent>().Mesh = mesh;
		}

		return true;
	}

	Ref<StaticMesh> Importer3D::ImportStaticMesh(const Filepath& filepath, uint32 aiMeshIndex)
	{
		const aiScene* aiscene = OpenFile(filepath);
		if (!aiscene)
			return nullptr;

		Ref<StaticMesh> mesh = ParseStaticMesh(aiscene, aiMeshIndex, m_CurrentFilepath);

		return mesh;
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
