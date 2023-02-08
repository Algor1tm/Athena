#include "Mesh.h"

#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Vertex.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/matrix4x4.h>


namespace Athena
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

	static Ref<Texture2D> LoadTexture(const aiScene* aiscene, const aiMaterial* aimaterial, uint32 type, const Filepath& path)
	{
		Ref<Texture2D> result = nullptr;

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(type, 0), texFilepath))
		{
			const aiTexture* embeddedTex = aiscene->GetEmbeddedTexture(texFilepath.C_Str());
			if (embeddedTex)
			{
				Texture2DDescription texDesc;
				texDesc.Data = embeddedTex->pcData;
				texDesc.Width = embeddedTex->mWidth;
				texDesc.Height = embeddedTex->mHeight;
				result = Texture2D::Create(texDesc);
			}
			else
			{
				Filepath path = path;
				path.replace_filename(texFilepath.C_Str());
				if (std::filesystem::exists(path))
				{
					result = Texture2D::Create(path);
				}
				else
				{
					ATN_CORE_WARN("Failed to load texture at {}", path);
				}
			}
		}

		return result;
	}

	static Ref<Material> LoadMaterial(const aiScene* aiscene, uint32 aiMaterialIndex, const Filepath& path)
	{
		Ref<Material> result;
		const aiMaterial* aimaterial = aiscene->mMaterials[aiMaterialIndex];
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


		if (desc.AlbedoMap = LoadTexture(aiscene, aimaterial, aiTextureType_BASE_COLOR, path))
			desc.UseAlbedoMap = true;
		else if (desc.AlbedoMap = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE, path))
			desc.UseAlbedoMap = true;

		if (desc.NormalMap = LoadTexture(aiscene, aimaterial, aiTextureType_NORMALS, path))
			desc.UseNormalMap = true;

		if (desc.RoughnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE_ROUGHNESS, path))
			desc.UseRoughnessMap = true;
		else if (desc.RoughnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_SHININESS, path))
			desc.UseRoughnessMap = true;

		if (desc.MetalnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_METALNESS, path))
			desc.UseMetalnessMap = true;

		if (desc.AmbientOcclusionMap = LoadTexture(aiscene, aimaterial, aiTextureType_AMBIENT_OCCLUSION, path))
			desc.UseAmbientOcclusionMap = true;
		else if (desc.AmbientOcclusionMap = LoadTexture(aiscene, aimaterial, aiTextureType_LIGHTMAP, path))
			desc.UseAmbientOcclusionMap = true;

		result = MaterialManager::CreateMaterial(desc, materialName);
		return result;
	}

	static Ref<IndexBuffer> LoadIndexBuffer(const aiMesh* aimesh)
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

		return IndexBuffer::Create(indicies.data(), indicies.size());
	}

	static Ref<VertexBuffer> LoadStaticVertexBuffer(const aiMesh* aimesh, const Matrix4& localTransform)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<StaticVertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				vertices[i].Position = ConvertaiVector3D(aimesh->mVertices[i]) * localTransform;
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
				vertices[i].Normal = Vector4(ConvertaiVector3D(aimesh->mNormals[i]), 0) * localTransform;
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				vertices[i].Tangent = Vector4(ConvertaiVector3D(aimesh->mTangents[i]), 0) * localTransform;
				// Bitangent
				vertices[i].Bitangent = Vector4(ConvertaiVector3D(aimesh->mBitangents[i]), 0) * localTransform;
			}
		}

		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(StaticVertex);
		vBufferDesc.Layout = StaticVertex::GetLayout();
		vBufferDesc.IndexBuffer = LoadIndexBuffer(aimesh);
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}

	static Ref<VertexBuffer> LoadAnimVertexBuffer(const aiMesh* aimesh, const Matrix4& localTransform, const Ref<Skeleton>& skeleton)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<AnimVertex> vertices(numVertices);

		if (aimesh->HasBones() && skeleton)
		{
			for (uint32 i = 0; i < aimesh->mNumBones; ++i)
			{
				aiBone* aibone = aimesh->mBones[i];
				skeleton->SetBoneOffsetMatrix(aibone->mName.C_Str(), ConvertaiMatrix4x4(aibone->mOffsetMatrix));
				uint32 boneID = skeleton->GetBoneID(aibone->mName.C_Str());

				for (uint32 j = 0; j < aibone->mNumWeights; ++j)
				{
					uint32 vertexID = aibone->mWeights[j].mVertexId;
					for (uint32 k = 0; k < MAX_NUM_BONES_PER_VERTEX; ++k)
					{
						if (vertices[vertexID].Weights[k] == 0.f)
						{
							vertices[vertexID].BoneIDs[k] = boneID;
							vertices[vertexID].Weights[k] = aibone->mWeights[j].mWeight;
							break;
						}
						else if (k == MAX_NUM_BONES_PER_VERTEX - 1)
						{
							ATN_CORE_WARN("Vertex has more than four bones/weights affecting it, extra data will be dicarded(BoneID = {}, Weight = {})",
								boneID, aibone->mWeights[j].mWeight);
						}
					}
				}
			}
		}

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				vertices[i].Position = ConvertaiVector3D(aimesh->mVertices[i]) * localTransform;
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
				vertices[i].Normal = Vector4(ConvertaiVector3D(aimesh->mNormals[i]), 0) * localTransform;
			}

			if (aimesh->HasTangentsAndBitangents())
			{
				// Tangent
				vertices[i].Tangent = Vector4(ConvertaiVector3D(aimesh->mTangents[i]), 0) * localTransform;
				// Bitangent
				vertices[i].Bitangent = Vector4(ConvertaiVector3D(aimesh->mBitangents[i]), 0) * localTransform;
			}
		}

		VertexBufferDescription vBufferDesc;
		vBufferDesc.Data = vertices.data();
		vBufferDesc.Size = vertices.size() * sizeof(AnimVertex);
		vBufferDesc.Layout = AnimVertex::GetLayout();
		vBufferDesc.IndexBuffer = LoadIndexBuffer(aimesh);
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}

	static SubMesh LoadSubMesh(const aiScene* aiscene, uint32 aiMeshIndex, const Filepath& path, const Matrix4& localTransform, Ref<Skeleton> skeleton, AABB& aabb)
	{
		aiMesh* aimesh = aiscene->mMeshes[aiMeshIndex];
		SubMesh subMesh;

		aabb.Extend(AABB(ConvertaiVector3D(aimesh->mAABB.mMin) * localTransform, ConvertaiVector3D(aimesh->mAABB.mMax) * localTransform));

		subMesh.Name = aimesh->mName.C_Str();
		if(skeleton)
			subMesh.VertexBuffer = LoadAnimVertexBuffer(aimesh, localTransform, skeleton);
		else
			subMesh.VertexBuffer = LoadStaticVertexBuffer(aimesh, localTransform);

		subMesh.MaterialName = LoadMaterial(aiscene, aimesh->mMaterialIndex, path)->GetName();

		return subMesh;
	}

	static void LoadBonesHierarchy(const aiNode* bonesRoot, BonesHierarchyInfo& info)
	{
		String nodeName = bonesRoot->mName.C_Str();
		bool isValid = nodeName.find("AssimpFbx") == String::npos && nodeName.find("RootNode") == String::npos;
		if (bonesRoot->mNumChildren > 0 && !isValid)
		{
			LoadBonesHierarchy(bonesRoot->mChildren[0], info);
			return;
		}

		info.Name = nodeName;
		info.Children.resize(bonesRoot->mNumChildren);
		for (uint32 i = 0; i < bonesRoot->mNumChildren; ++i)
		{
			LoadBonesHierarchy(bonesRoot->mChildren[i], info.Children[i]);
		}
	}

	static Ref<Skeleton> LoadSkeleton(const aiScene* aiscene)
	{
		const aiNode* root = aiscene->mRootNode;
		const aiNode* bonesRoot = nullptr;

		for (uint32 i = 0; i < root->mNumChildren; ++i)
		{
			String name = root->mChildren[i]->mName.C_Str();
			if (name.find("RootNode") != String::npos && aiscene->mNumAnimations > 0)
			{
				bonesRoot = root->mChildren[i];
				break;
			}
		}
		if (bonesRoot == nullptr)
			return nullptr;

		BonesHierarchyInfo info;
		LoadBonesHierarchy(bonesRoot, info);

		return Skeleton::Create(info);
	}

	static Ref<Animation> LoadAnimation(const aiAnimation* aianimation, const Ref<Skeleton>& skeleton)
	{
		AnimationDescription desc;
		desc.Name = aianimation->mName.C_Str();
		desc.Duration = aianimation->mDuration;
		desc.TicksPerSecond = aianimation->mTicksPerSecond;
		desc.Skeleton = skeleton;

		ATN_CORE_ASSERT(aianimation->mNumChannels < MAX_NUM_BONES);

		desc.BoneNameToKeyFramesMap.reserve(aianimation->mNumChannels);
		for (uint32 i = 0; i < aianimation->mNumChannels; ++i)
		{
			aiNodeAnim* channel = aianimation->mChannels[i];
			ATN_CORE_ASSERT((channel->mNumPositionKeys == channel->mNumRotationKeys) && (channel->mNumRotationKeys == channel->mNumScalingKeys));

			std::vector<KeyFrame> keyFrames(channel->mNumPositionKeys);
			for (uint32 j = 0; j < channel->mNumPositionKeys; ++j)
			{
				keyFrames[j].TimeStamp = channel->mPositionKeys[j].mTime;

				keyFrames[j].Translation = ConvertaiVector3D(channel->mPositionKeys[j].mValue);
				keyFrames[j].Rotation = ConvertaiQuaternion(channel->mRotationKeys[j].mValue);
				keyFrames[j].Scale = ConvertaiVector3D(channel->mScalingKeys[j].mValue);
			}

			desc.BoneNameToKeyFramesMap[channel->mNodeName.C_Str()] = keyFrames;
		}

		return Animation::Create(desc);
	}

	void StaticMesh::ProcessNode(const aiScene* aiscene, const aiNode* ainode, const Matrix4& parentTransform)
	{
		Matrix4 localTransform = parentTransform * ConvertaiMatrix4x4(ainode->mTransformation);

		m_SubMeshes.reserve(ainode->mNumMeshes);
		for (uint32 i = 0; i < ainode->mNumMeshes; ++i)
		{
			SubMesh subMesh = LoadSubMesh(aiscene, ainode->mMeshes[i], m_Filepath, localTransform, m_Skeleton, m_AABB);
			m_SubMeshes.push_back(subMesh);
		}

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			ProcessNode(aiscene, ainode->mChildren[i], localTransform);
		}
	}

	Ref<StaticMesh> StaticMesh::Create(const Filepath& path)
	{
		const unsigned int flags =
			aiProcess_OptimizeGraph |
			aiProcess_Triangulate |
			aiProcess_GenUVCoords |
			aiProcess_CalcTangentSpace |
			aiProcess_GenNormals |
			aiProcess_SortByPType |
			aiProcess_FindDegenerates |
			aiProcess_ImproveCacheLocality |
			aiProcess_LimitBoneWeights |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_OptimizeMeshes |
			aiProcess_EmbedTextures |
			aiProcess_GenBoundingBoxes;

		const aiScene* aiscene = aiImportFile(path.string().c_str(), flags);

		if (aiscene == nullptr)
		{
			const char* error = aiGetErrorString();
			ATN_CORE_ERROR("Importer3D Error: {0}", error);
			return nullptr;
		}

		Ref<StaticMesh> result = CreateRef<StaticMesh>();
		result->m_Filepath = path;
		result->m_Name = path.stem().string();

		result->m_Skeleton = LoadSkeleton(aiscene);

		const aiNode* root = aiscene->mRootNode;
		for (uint32 i = 0; i < root->mNumChildren; ++i)
		{
			result->ProcessNode(aiscene, root->mChildren[i], Matrix4::Identity());
		}

		if (aiscene->mNumAnimations > 0)
		{
			std::vector<Ref<Animation>> animations(aiscene->mNumAnimations);

			for (uint32 i = 0; i < aiscene->mNumAnimations; ++i)
			{
				animations[i] = LoadAnimation(aiscene->mAnimations[i], result->m_Skeleton);
			}

			result->m_Animator = Animator::Create(animations);
		}

		aiReleaseImport(aiscene);

		return result;
	}
}