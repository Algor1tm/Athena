#include "Importer3D.h"

#include "Athena/Renderer/Material.h"
#include "Athena/Renderer/Vertex.h"
#include "Athena/Renderer/RendererAPI.h"

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

	static Quaternion aiQuaternionToQuaternion(const aiQuaternion& quat)
	{
		return { quat.w, quat.x, quat.y, quat.z };
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

	Ref<Texture2D> Importer3D::LoadTexture(const aiScene* aiscene, const aiMaterial* aimaterial, uint32 type)
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
				Filepath path = m_SceneFilepath;
				path.replace_filename(texFilepath.C_Str());
				if (std::filesystem::exists(path))
				{
					result = Texture2D::Create(path);
				}
				else
				{
					ATN_CORE_WARN("Importer3D: Failed to load texture at {}", path);
				}
			}
		}

		return result;
	}

	Ref<Material> Importer3D::LoadMaterial(const aiScene* aiscene, uint32 aiMaterialIndex)
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


		if (desc.AlbedoMap = LoadTexture(aiscene, aimaterial, aiTextureType_BASE_COLOR))
			desc.UseAlbedoMap = true;
		else if (desc.AlbedoMap = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE))
			desc.UseAlbedoMap = true;

		if (desc.NormalMap = LoadTexture(aiscene, aimaterial, aiTextureType_NORMALS))
			desc.UseNormalMap = true;

		if (desc.RoughnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_DIFFUSE_ROUGHNESS))
			desc.UseRoughnessMap = true;
		else if (desc.RoughnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_SHININESS))
			desc.UseRoughnessMap = true;

		if (desc.MetalnessMap = LoadTexture(aiscene, aimaterial, aiTextureType_METALNESS))
			desc.UseMetalnessMap = true;

		if (desc.AmbientOcclusionMap = LoadTexture(aiscene, aimaterial, aiTextureType_AMBIENT_OCCLUSION))
			desc.UseAmbientOcclusionMap = true;
		else if (desc.AmbientOcclusionMap = LoadTexture(aiscene, aimaterial, aiTextureType_LIGHTMAP))
			desc.UseAmbientOcclusionMap = true;

		result = MaterialManager::CreateMaterial(desc, materialName);
		return result;
	}

	Ref<IndexBuffer> Importer3D::LoadIndexBuffer(const aiMesh* aimesh)
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

	Ref<VertexBuffer> Importer3D::LoadStaticVertexBuffer(const aiMesh* aimesh)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<StaticVertex> vertices(numVertices);

		for (uint32 i = 0; i < numVertices; ++i)
		{
			// Position
			if (aimesh->HasPositions())
			{
				CopyaiVector3DToVector3(aimesh->mVertices[i], vertices[i].Position);
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
		vBufferDesc.Size = vertices.size() * sizeof(StaticVertex);
		vBufferDesc.Layout = StaticVertex::GetLayout();
		vBufferDesc.IndexBuffer = LoadIndexBuffer(aimesh);
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}

	Ref<VertexBuffer> Importer3D::LoadAnimVertexBuffer(const aiMesh* aimesh, const Ref<Skeleton>& skeleton)
	{
		uint32 numVertices = aimesh->mNumVertices;
		std::vector<AnimVertex> vertices(numVertices);

		if (aimesh->HasBones() && skeleton)
		{
			for (uint32 i = 0; i < aimesh->mNumBones; ++i)
			{
				aiBone* aibone = aimesh->mBones[i];
				skeleton->SetBoneOffsetMatrix(aibone->mName.C_Str(), aiMatrix4x4ToMatrix4(aibone->mOffsetMatrix));
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
				CopyaiVector3DToVector3(aimesh->mVertices[i], vertices[i].Position);
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
		vBufferDesc.Size = vertices.size() * sizeof(AnimVertex);
		vBufferDesc.Layout = AnimVertex::GetLayout();
		vBufferDesc.IndexBuffer = LoadIndexBuffer(aimesh);
		vBufferDesc.Usage = BufferUsage::STATIC;

		return VertexBuffer::Create(vBufferDesc);
	}

	Ref<Animation> Importer3D::LoadAnimation(const aiAnimation* aianimation, const Ref<Skeleton>& skeleton)
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

				keyFrames[j].Translation = aiVector3DToVector3(channel->mPositionKeys[j].mValue);
				keyFrames[j].Rotation = aiQuaternionToQuaternion(channel->mRotationKeys[j].mValue);
				keyFrames[j].Scale = aiVector3DToVector3(channel->mScalingKeys[j].mValue);
			}

			desc.BoneNameToKeyFramesMap[channel->mNodeName.C_Str()] = keyFrames;
		}

		return Animation::Create(desc);
	}

	Ref<Skeleton> Importer3D::LoadSkeleton(const aiScene* aiscene)
	{
		BoneStructureInfo rootBone;

		const aiNode* rootNode = aiscene->mRootNode;
		for (uint32 i = 0; i < rootNode->mNumChildren; ++i)
		{
			LoadSkeletonRecursiveHelper(rootNode->mChildren[i], rootBone);
		}

		if (rootBone.Children.empty() && rootBone.Name.empty())
			return nullptr;

		return Skeleton::Create(rootBone);
	}

	void Importer3D::LoadSkeletonRecursiveHelper(const aiNode* ainode, BoneStructureInfo& bone)
	{
		if (ainode->mNumMeshes == 0)
		{
			String nodeName = ainode->mName.C_Str();
			bool isValid = nodeName.find("AssimpFbx") == String::npos && nodeName.find("RootNode") == String::npos;
			if (ainode->mNumChildren > 0 && !isValid)
			{
				LoadSkeletonRecursiveHelper(ainode->mChildren[0], bone);
				return;
			}

			bone.Name = nodeName;
			bone.Children.resize(ainode->mNumChildren);
			for (uint32 i = 0; i < ainode->mNumChildren; ++i)
			{
				LoadSkeletonRecursiveHelper(ainode->mChildren[i], bone.Children[i]);
			}
		}
	}

	Ref<SkeletalMesh> Importer3D::LoadSkeletalMesh(const aiScene* aiscene)
	{
		Ref<Skeleton> skeleton = LoadSkeleton(aiscene);

		std::vector<SubMesh> subMeshes;
		SceneNode node;
		node.aiNode = aiscene->mRootNode;
		Matrix4 globalInverse = aiMatrix4x4ToMatrix4(aiscene->mRootNode->mTransformation);
		ProcessNodeForSubMeshes(aiscene, &node, subMeshes, skeleton);

		std::vector<Ref<Animation>> animations(aiscene->mNumAnimations);
		if (skeleton)
		{
			for (uint32 i = 0; i < aiscene->mNumAnimations; ++i)
				animations[i] = LoadAnimation(aiscene->mAnimations[i], skeleton);
		}

		SkeletalMeshDescription desc;
		desc.Name = m_SceneFilepath.stem().string();
		desc.Filepath = m_SceneFilepath;
		desc.SubMeshes = subMeshes;
		desc.Skeleton = skeleton;
		desc.Animations = animations;

		return SkeletalMesh::Create(desc);
	}

	Ref<StaticMesh> Importer3D::LoadStaticMesh(const aiScene* aiscene, uint32 aiMeshIndex)
	{
		aiMesh* aimesh = aiscene->mMeshes[aiMeshIndex];

		Ref<StaticMesh> result = CreateRef<StaticMesh>();

		result->BoundingBox = AABB(aiVector3DToVector3(aimesh->mAABB.mMin), aiVector3DToVector3(aimesh->mAABB.mMax));
		result->Name = aimesh->mName.C_Str();
		result->MaterialName = LoadMaterial(aiscene, aimesh->mMaterialIndex)->GetName();
		result->Filepath = m_SceneFilepath;
		result->aiMeshIndex = aiMeshIndex;

		result->Vertices = LoadStaticVertexBuffer(aimesh);

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

		if (aiscene->HasAnimations() || aiscene->mNumMaterials != aiscene->mNumMeshes)
		{
			Ref<SkeletalMesh> mesh = LoadSkeletalMesh(aiscene);

			Entity entity = m_Scene->CreateEntity();
			entity.GetComponent<TagComponent>().Tag = mesh->GetName();
			entity.AddComponent<SkeletalMeshComponent>().Mesh = mesh;
			entity.GetComponent<SkeletalMeshComponent>().Animator = Animator::Create(mesh);
		}
		else
		{
			ProcessNodeForStaticMeshes(aiscene, &root);
		}

		return true;
	}

	Ref<StaticMesh> Importer3D::ImportStaticMesh(const Filepath& path, uint32 aiMeshIndex)
	{
		const aiScene* aiscene = OpenFile(path);
		if (!aiscene)
			return nullptr;

		Ref<StaticMesh> mesh = LoadStaticMesh(aiscene, aiMeshIndex);

		return mesh;
	}

	Ref<SkeletalMesh> Importer3D::ImportSkeletalMesh(const Filepath& path)
	{
		const aiScene* aiscene = OpenFile(path);
		if (!aiscene)
			return nullptr;

		Ref<SkeletalMesh> mesh = LoadSkeletalMesh(aiscene);

		return mesh;
	}

	void Importer3D::ProcessNodeForStaticMeshes(const aiScene* aiscene, const SceneNode* node)
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

			ProcessNodeForStaticMeshes(aiscene, &newNode);
		}
	}

	void Importer3D::ProcessNodeForSubMeshes(const aiScene* aiscene, const SceneNode* node, std::vector<SubMesh>& subMeshes, const Ref<Skeleton>& skeleton)
	{
		aiMatrix4x4 localTransform = node->WorldTransform * node->aiNode->mTransformation;

		subMeshes.reserve(node->aiNode->mNumMeshes);
		for (uint32 i = 0; i < node->aiNode->mNumMeshes; ++i)
		{
			aiMesh* aimesh = aiscene->mMeshes[node->aiNode->mMeshes[i]];

			SubMesh mesh;
			mesh.Name = aimesh->mName.C_Str();
			mesh.VertexBuffer = LoadAnimVertexBuffer(aimesh, skeleton);
			mesh.MaterialName = LoadMaterial(aiscene, aimesh->mMaterialIndex)->GetName();
			mesh.LocalTransform = aiMatrix4x4ToMatrix4(localTransform);
			subMeshes.push_back(mesh);
		}

		for (uint32 i = 0; i < node->aiNode->mNumChildren; ++i)
		{
			SceneNode newNode;
			newNode.WorldTransform = localTransform;

			if (node->aiNode->mChildren[i]->mNumMeshes > 0)
			{
				newNode.aiNode = node->aiNode->mChildren[i];
				ProcessNodeForSubMeshes(aiscene, &newNode, subMeshes, skeleton);
			}
		}
	}

	const aiScene* Importer3D::OpenFile(const Filepath& path)
	{
		if (m_ImportedScenes.find(path) == m_ImportedScenes.end())
		{
			unsigned int flags =
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
				aiProcess_LimitBoneWeights |
				aiProcess_GenBoundingBoxes;

			if (RendererAPI::GetAPI() == RendererAPI::Direct3D)
				flags |= aiProcess_MakeLeftHanded | aiProcess_FlipUVs;

			const aiScene* aiscene = aiImportFile(path.string().c_str(), flags);

			if (aiscene == nullptr || aiscene->mFlags == AI_SCENE_FLAGS_INCOMPLETE)
			{
				aiReleaseImport(aiscene);
				const char* error = aiGetErrorString();
				ATN_CORE_ERROR("Importer3D Error: {0}", error);
				return nullptr;
			}

			m_ImportedScenes[path] = aiscene;
		}

		m_SceneFilepath = path;

		return m_ImportedScenes[path];
	}

	void Importer3D::Release()
	{
		for (auto& [_, aiscene] : m_ImportedScenes)
		{
			aiReleaseImport(aiscene);
		}
	}
}
