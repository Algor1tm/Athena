#include "AssimpImporter.h"

#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/TextureImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Project/Project.h"
#include "Athena/Renderer/Renderer.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>


namespace Athena
{
	namespace Utils
	{
		static Matrix4 ConvertaiMatrix4x4(const aiMatrix4x4& input)
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

		static Quaternion ConvertaiQuaternion(const aiQuaternion& quat)
		{
			return { quat.w, quat.x, quat.y, quat.z };
		}

		static Vector3 ConvertaiVector3D(const aiVector3D& input)
		{
			return { input.x, input.y, input.z };
		}

		static String ConvertaiStringName(const aiString& aiName)
		{
			const uint32 nameMaxLength = 30;

			if (aiName.length >= nameMaxLength)
			{
				String name = aiName.C_Str();
				return name.substr(0, nameMaxLength);
			}

			return aiName.C_Str();
		}
	}

	static const unsigned int flags =
		aiProcess_GenUVCoords |
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals |
		aiProcess_FixInfacingNormals |
		aiProcess_GenBoundingBoxes |
		aiProcess_FindInvalidData |

		aiProcess_SortByPType |
		aiProcess_FindDegenerates |
		aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices |
		aiProcess_LimitBoneWeights |

		aiProcess_RemoveRedundantMaterials |
		aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes |

		aiProcess_Triangulate |
		aiProcess_FlipUVs;

	AssimpImporter::AssimpImporter(const FilePath& path)
		: m_aiScene(aiImportFile(path.string().c_str(), flags))
	{
		m_Path = path;

		if (m_aiScene == nullptr)
		{
			const char* error = aiGetErrorString();
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to import mesh from {}.", m_Path);
			ATN_CORE_ERROR("	Assimp Error: {}", error);
		}
	}

	AssimpImporter::~AssimpImporter()
	{
		if(m_aiScene)
			aiReleaseImport(m_aiScene);
	}

	Ref<MeshSource> AssimpImporter::ImportToMeshSource() const
	{
		if (!m_aiScene)
			return nullptr;

		Ref<MeshSource> meshSource = MeshSource::Create();
		TraverseNodes(meshSource, m_aiScene->mRootNode);

		return meshSource;
	}

	void AssimpImporter::TraverseNodes(const Ref<MeshSource>& meshSource, const aiNode* ainode, const Matrix4& parentTransform) const
	{
		Matrix4 transform = parentTransform * Utils::ConvertaiMatrix4x4(ainode->mTransformation);

		for (uint32 i = 0; i < ainode->mNumMeshes; ++i)
		{
			uint32 aiMeshIndex = ainode->mMeshes[i];
			SubMesh subMesh = LoadSubMesh(meshSource, m_aiScene->mMeshes[aiMeshIndex], transform);
			meshSource->m_SubMeshes.push_back(subMesh);
		}

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			TraverseNodes(meshSource, ainode->mChildren[i], transform);
		}
	}

	SubMesh AssimpImporter::LoadSubMesh(const Ref<MeshSource>& meshSource, const aiMesh* aimesh, const Matrix4& transform) const
	{
		SubMesh subMesh;
		// AABB
		subMesh.AABB = AABB(Utils::ConvertaiVector3D(aimesh->mAABB.mMin) * transform, Utils::ConvertaiVector3D(aimesh->mAABB.mMax) * transform);
		meshSource->m_AABB.Extend(subMesh.AABB);

		// VertexBuffer
		subMesh.Name = aimesh->mName.C_Str();
		subMesh.VertexBuffer = LoadStaticVertexBuffer(aimesh, transform);

		// Matertial
		const aiMaterial* aimaterial = m_aiScene->mMaterials[aimesh->mMaterialIndex];
		subMesh.MaterialName = aimaterial->GetName().C_Str();
		MaterialTable& matTable = meshSource->m_MaterialTable;

		if (!matTable.contains(subMesh.MaterialName))
		{
			matTable[subMesh.MaterialName] = LoadMaterial(aimaterial);
		}

		return subMesh;
	}

	Ref<VertexBuffer> AssimpImporter::LoadStaticVertexBuffer(const aiMesh* aimesh, const Matrix4& transform) const
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<StaticVertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				vertices[i].Position = Utils::ConvertaiVector3D(aimesh->mVertices[i]) * transform;
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
				vertices[i].Normal = Vector4(Utils::ConvertaiVector3D(aimesh->mNormals[i]), 0) * transform;
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				vertices[i].Tangent = Vector4(Utils::ConvertaiVector3D(aimesh->mTangents[i]), 0) * transform;
				// Bitangent
				vertices[i].Bitangent = Vector4(Utils::ConvertaiVector3D(aimesh->mBitangents[i]), 0) * transform;
			}
		}

		uint32 numFaces = aimesh->mNumFaces;
		aiFace* faces = aimesh->mFaces;

		std::vector<uint32> indices(numFaces * 3);

		uint32 index = 0;
		for (uint32 i = 0; i < numFaces; i++)
		{
			if (faces[i].mNumIndices != 3)
				break;

			indices[index++] = faces[i].mIndices[0];
			indices[index++] = faces[i].mIndices[1];
			indices[index++] = faces[i].mIndices[2];
		}

		Ref<IndexBuffer> indexBuffer = nullptr;
		if (!indices.empty())
		{
			IndexBufferCreateInfo indexBufferInfo;
			indexBufferInfo.Name = std::format("{}_IndexBuffer", Utils::ConvertaiStringName(aimesh->mName));
			indexBufferInfo.Data = indices.data();
			indexBufferInfo.Count = indices.size();
			indexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			indexBuffer = IndexBuffer::Create(indexBufferInfo);
		}

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = std::format("{}_VertexBuffer", Utils::ConvertaiStringName(aimesh->mName));
		vertexBufferInfo.Data = vertices.data();
		vertexBufferInfo.Size = vertices.size() * sizeof(StaticVertex);
		vertexBufferInfo.IndexBuffer = indexBuffer;
		vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

		return VertexBuffer::Create(vertexBufferInfo);
	}

	AssetHandle AssimpImporter::LoadMaterial(const aiMaterial* aimaterial) const
	{
		Ref<MaterialAsset> material = MaterialAsset::Create();
		AssetHandle materialHandle = Project::GetEditorAssetManager()->AddMemoryOnlyAsset(material);

		aiColor4D color;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_BASE_COLOR, color))
			material->SetAlbedo(LinearColor(color.r, color.g, color.b, color.a));
		else if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			material->SetAlbedo(LinearColor(color.r, color.g, color.b, color.a));

		float roughness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness))
			material->SetRoughness(roughness);
		else if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_GLOSSINESS_FACTOR, roughness))
			material->SetRoughness(1 - roughness);

		float metalness;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_METALLIC_FACTOR, metalness))
			material->SetMetalness(metalness);

		float emission;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emission))
			material->SetEmission(emission);

		AssetHandle texture = 0;

		// Albedo 
		if (texture = LoadMaterialTexture(aimaterial, aiTextureType_BASE_COLOR, true))
			material->SetTexture(MaterialTextureType::Albedo, texture);
		else if (texture = LoadMaterialTexture(aimaterial, aiTextureType_DIFFUSE, true))
			material->SetTexture(MaterialTextureType::Albedo, texture);

		material->EnableTexture(MaterialTextureType::Albedo, (texture != 0));

		// Normal
		if (texture = LoadMaterialTexture(aimaterial, aiTextureType_NORMALS, false))
			material->SetTexture(MaterialTextureType::Normals, texture);

		material->EnableTexture(MaterialTextureType::Normals, (texture != 0));

		// Roughness
		if (texture = LoadMaterialTexture(aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, false))
			material->SetTexture(MaterialTextureType::Roughness, texture);
		else if (texture = LoadMaterialTexture(aimaterial, aiTextureType_SHININESS, false))
			material->SetTexture(MaterialTextureType::Roughness, texture);

		material->EnableTexture(MaterialTextureType::Roughness, (texture != 0));

		// Metalness
		if (texture = LoadMaterialTexture(aimaterial, aiTextureType_METALNESS, false))
			material->SetTexture(MaterialTextureType::Metalness, texture);

		material->EnableTexture(MaterialTextureType::Metalness, (texture != 0));

		return materialHandle;
	}

	AssetHandle AssimpImporter::LoadMaterialTexture(const aiMaterial* aimaterial, uint32 type, bool srgb) const
	{
		AssetHandle handle = 0;

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(type, 0), texFilepath))
		{
			TextureImportOptions options;
			options.sRGB = srgb;
			options.GenerateMipMaps = true;

			const aiTexture* embeddedTex = m_aiScene->GetEmbeddedTexture(texFilepath.C_Str());
			if (embeddedTex)
			{
				void* data = embeddedTex->pcData;
				uint32 width = embeddedTex->mWidth;
				uint32 height = embeddedTex->mHeight;

				options.Name = String(texFilepath.C_Str(), texFilepath.length);
				Ref<Texture2D> texture = TextureImporter::LoadFromMemory(data, width, height, options);
				if (texture)
				{
					Ref<TextureAsset> textureAsset = Ref<TextureAsset>::Create(texture);
					handle = Project::GetEditorAssetManager()->AddMemoryOnlyAsset(textureAsset);
				}
			}
			else
			{
				FilePath path = m_Path;
				path.replace_filename(texFilepath.C_Str());

				handle = Project::GetEditorAssetManager()->GetAssetHandleFromFilePath(path);
				if (!handle)
				{
					ATN_CORE_WARN_TAG("AssetManager", "Failed to find texture with filepath {} while importing MeshSource", path);
				}
			}
		}

		return handle;
	}
}
