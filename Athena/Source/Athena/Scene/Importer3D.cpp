#include "Importer3D.h"

#include "Athena/Renderer/Renderer.h"
#include "Athena/Renderer/Material.h"

#include "Entity.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>


namespace Athena
{
	struct SceneNode
	{
		aiNode* aiNode = nullptr;
		aiMatrix4x4 WorldTransform;
	};

	static Matrix4 aiMatrix4x4ToMatrix4(const aiMatrix4x4& input)
	{
		Matrix4 output;

		for (uint32 i = 0; i < 4; ++i)
		{
			for (uint32 j = 0; j < 4; ++j)
			{
				output[i][j] = input[j][i];
			}
		}

		return output;
	}

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

	Ref<Texture2D> Importer3D::LoadTexture(const aiScene* scene, const aiMaterial* aimaterial, uint32 type)
	{
		Ref<Texture2D> result = nullptr;

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(type, 0), texFilepath))
		{
			const aiTexture* embededTex = scene->GetEmbeddedTexture(texFilepath.C_Str());
			if (embededTex)
			{
				Texture2DDescription texDesc;
				texDesc.Data = embededTex->pcData;
				texDesc.Width = embededTex->mWidth;
				texDesc.Height = embededTex->mHeight;
				result = Texture2D::Create(texDesc);
			}
			else
			{
				Filepath path = m_SceneFilepath;
				path.replace_filename(texFilepath.C_Str());
				result = Texture2D::Create(path);
			}
		}

		return result;
	}

	Ref<Material> Importer3D::LoadMaterial(const aiScene* scene, uint32 aiMaterialIndex)
	{
		Ref<Material> result;
		const aiMaterial* aimaterial = scene->mMaterials[aiMaterialIndex];
		String materialName = aimaterial->GetName().C_Str();

		if (result = MaterialManager::GetMaterial(materialName))
			return result;

		MaterialDescription desc;

		aiColor4D color;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_BASE_COLOR, color))
			desc.Albedo = Vector3(color.r, color.g, color.b);
		else if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			desc.Albedo = Vector3(color.r, color.g, color.b);

		float roughness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			desc.Roughness = roughness;

		float metalness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
			desc.Metalness = metalness;


		if (desc.AlbedoMap = LoadTexture(scene, aimaterial, aiTextureType_BASE_COLOR))
			desc.UseAlbedoMap = true;
		else if (desc.AlbedoMap = LoadTexture(scene, aimaterial, aiTextureType_DIFFUSE))
			desc.UseAlbedoMap = true;

		if (desc.NormalMap = LoadTexture(scene, aimaterial, aiTextureType_NORMALS))
			desc.UseNormalMap = true;

		if (desc.RoughnessMap = LoadTexture(scene, aimaterial, aiTextureType_DIFFUSE_ROUGHNESS))
			desc.UseRoughnessMap = true;
		else if (desc.RoughnessMap = LoadTexture(scene, aimaterial, aiTextureType_SHININESS))
			desc.UseRoughnessMap = true;

		if (desc.MetalnessMap = LoadTexture(scene, aimaterial, aiTextureType_METALNESS))
			desc.UseMetalnessMap = true;

		if (desc.AmbientOcclusionMap = LoadTexture(scene, aimaterial, aiTextureType_AMBIENT_OCCLUSION))
			desc.UseAmbientOcclusionMap = true;
		else if (desc.AmbientOcclusionMap = LoadTexture(scene, aimaterial, aiTextureType_LIGHTMAP))
			desc.UseAmbientOcclusionMap = true;

		result = MaterialManager::CreateMaterial(desc, materialName);
		return result;
	}

	Ref<StaticMesh> Importer3D::LoadStaticMesh(const aiScene* scene, uint32 aiMeshIndex)
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
		result->MaterialName = LoadMaterial(scene, aimesh->mMaterialIndex)->GetName();
		result->Filepath = m_SceneFilepath;
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
			for (uint32 j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
			{
				if (aimesh->HasTextureCoords(j))
				{
					vertices[i].TexCoords.x = aimesh->mTextureCoords[j][i].x;
					vertices[i].TexCoords.y = aimesh->mTextureCoords[j][i].y;
					break;
				}
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

	Importer3D::Importer3D(const Ref<Scene>& scene)
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

		SceneNode root;
		root.aiNode = aiscene->mRootNode;

		ProcessNode(aiscene, &root);

		return true;
	}

	Ref<StaticMesh> Importer3D::ImportStaticMesh(const Filepath& filepath, uint32 aiMeshIndex)
	{
		const aiScene* aiscene = OpenFile(filepath);
		if (!aiscene)
			return nullptr;

		Ref<StaticMesh> mesh = LoadStaticMesh(aiscene, aiMeshIndex);

		return mesh;
	}

	void Importer3D::ProcessNode(const aiScene* aiscene, const SceneNode* node)
	{
		aiMatrix4x4 worldTransform = node->WorldTransform * node->aiNode->mTransformation;
		
		aiVector3D translation, scale, rotation;
		TransformComponent transform;

		if (node->aiNode->mNumMeshes > 0)
		{
			worldTransform.Decompose(scale, rotation, translation);

			transform.Translation = aiVector3DToVector3(translation);
			transform.Rotation = aiVector3DToVector3(rotation);;
			transform.Scale = aiVector3DToVector3(scale);
		}

		for (uint32 i = 0; i < node->aiNode->mNumMeshes; ++i)
		{
			Ref<StaticMesh> mesh = LoadStaticMesh(aiscene, node->aiNode->mMeshes[i]);

			Entity entity = m_Scene->CreateEntity();
			entity.GetComponent<TagComponent>().Tag = mesh->Name;
			entity.AddComponent<StaticMeshComponent>().Mesh = mesh;

			entity.GetComponent<TransformComponent>() = transform;
		}

		SceneNode newNode;
		newNode.WorldTransform = worldTransform;

		for (uint32 i = 0; i < node->aiNode->mNumChildren; ++i)
		{
			newNode.aiNode = node->aiNode->mChildren[i];

			ProcessNode(aiscene, &newNode);
		}
	}

	const aiScene* Importer3D::OpenFile(const Filepath& filepath)
	{
		if (m_ImportedScenes.find(filepath) == m_ImportedScenes.end())
		{
			unsigned int flags =
				aiProcess_Triangulate |
				aiProcess_GenUVCoords |
				aiProcess_CalcTangentSpace |
				aiProcess_GenNormals |
				aiProcess_SortByPType |
				aiProcess_FindDegenerates | 
				aiProcess_ImproveCacheLocality |
				aiProcess_LimitBoneWeights | 
				aiProcess_OptimizeGraph |
				aiProcess_RemoveRedundantMaterials |
				aiProcess_OptimizeMeshes |
				aiProcess_EmbedTextures | 
				aiProcess_GenBoundingBoxes;

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

		m_SceneFilepath = filepath;

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
