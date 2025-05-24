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

#include <set>

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

		static void PrintNodes(const aiNode* ainode, uint32 level = 0)
		{
			String msg = ainode->mName.C_Str();

			if (level > 0)
				msg = String(level, ' ') + msg;

			ATN_CORE_TRACE(msg);

			for (uint32 i = 0; i < ainode->mNumChildren; ++i)
			{
				PrintNodes(ainode->mChildren[i], level + 1);
			}
		}
	}

	static const unsigned int s_ImportFlags =
		aiProcess_GenUVCoords |
		aiProcess_CalcTangentSpace |
		aiProcess_GenNormals |
		aiProcess_FixInfacingNormals |
		aiProcess_GenBoundingBoxes |
		aiProcess_FindInvalidData |
		aiProcess_PopulateArmatureData |

		aiProcess_SortByPType |
		aiProcess_ImproveCacheLocality |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ValidateDataStructure |
		aiProcess_LimitBoneWeights |

		aiProcess_RemoveRedundantMaterials |
		//aiProcess_OptimizeGraph |
		aiProcess_OptimizeMeshes |

		aiProcess_Triangulate |
		aiProcess_FlipUVs;

	AssimpImporter::AssimpImporter(const FilePath& path)
		: m_aiScene(aiImportFile(path.string().c_str(), s_ImportFlags))
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

		if (HasSkeleton())
		{
			Ref<Skeleton> skeleton = ImportSkeleton();

			if (skeleton)
			{
				meshSource->m_IsRigged = true;
				meshSource->m_Skeleton = skeleton;

				meshSource->m_Animations.reserve(m_aiScene->mNumAnimations);
				for (uint32 i = 0; i < m_aiScene->mNumAnimations; ++i)
				{
					Ref<Animation> animation = ImportAnimation(i, skeleton);
					meshSource->m_Animations.push_back(animation);
				}
			}
		}

		MaterialTable& matTable = meshSource->m_MaterialTable;
		for (uint32 i = 0; i < m_aiScene->mNumMaterials; ++i)
		{
			aiMaterial* aimaterial = m_aiScene->mMaterials[i];
			String name = aimaterial->GetName().C_Str();

			matTable[name] = LoadMaterial(aimaterial);
		}

		LoadGeometry(meshSource);

		meshSource->m_Nodes.emplace_back();
		TraverseNodes(meshSource, m_aiScene->mRootNode);

		meshSource->m_Nodes[0].Name = m_Path.stem().string();

#if 0
		ATN_CORE_WARN("Node hierarchy for mesh {}", m_Path);
		Utils::PrintNodes(m_aiScene->mRootNode);
#endif

		return meshSource;
	}

	Ref<Animation> AssimpImporter::ImportAnimation(uint32 animationIndex, const Ref<Skeleton>& skeleton) const 
	{
		if (!m_aiScene || m_aiScene->mNumAnimations < animationIndex + 1)
			return nullptr;

		aiAnimation* aianimation = m_aiScene->mAnimations[animationIndex];

		AnimationCreateInfo info;
		info.Name = aianimation->mName.C_Str();
		info.Duration = aianimation->mDuration;
		info.TicksPerSecond = aianimation->mTicksPerSecond;
		info.Skeleton = skeleton;

		info.BoneNameToKeyFramesMap.reserve(aianimation->mNumChannels);
		for (uint32 i = 0; i < aianimation->mNumChannels; ++i)
		{
			aiNodeAnim* channel = aianimation->mChannels[i];
			KeyFramesList keyFrames;

			keyFrames.TranslationKeys.resize(channel->mNumPositionKeys);
			for (uint32 j = 0; j < channel->mNumPositionKeys; ++j)
			{
				keyFrames.TranslationKeys[j].TimeStamp = channel->mPositionKeys[j].mTime;
				keyFrames.TranslationKeys[j].Value = Utils::ConvertaiVector3D(channel->mPositionKeys[j].mValue);
			}

			keyFrames.RotationKeys.resize(channel->mNumRotationKeys);
			for (uint32 j = 0; j < channel->mNumRotationKeys; ++j)
			{
				keyFrames.RotationKeys[j].TimeStamp = channel->mRotationKeys[j].mTime;
				keyFrames.RotationKeys[j].Value = Utils::ConvertaiQuaternion(channel->mRotationKeys[j].mValue);
			}

			keyFrames.ScaleKeys.resize(channel->mNumScalingKeys);
			for (uint32 j = 0; j < channel->mNumScalingKeys; ++j)
			{
				keyFrames.ScaleKeys[j].TimeStamp = channel->mScalingKeys[j].mTime;
				keyFrames.ScaleKeys[j].Value = Utils::ConvertaiVector3D(channel->mScalingKeys[j].mValue);
			}

			info.BoneNameToKeyFramesMap[channel->mNodeName.C_Str()] = keyFrames;
		}

		return Animation::Create(info);
	}

	Ref<Skeleton> AssimpImporter::ImportSkeleton() const
	{
		if (!m_aiScene)
			return nullptr;

		std::unordered_map<String, Matrix4> bonesMap;

		for (uint32 i = 0; i < m_aiScene->mNumMeshes; ++i)
		{
			aiMesh* aimesh = m_aiScene->mMeshes[i];

			for (uint32 j = 0; j < aimesh->mNumBones; ++j)
			{
				aiBone* aibone = aimesh->mBones[j];
				bonesMap[aibone->mName.C_Str()] = Utils::ConvertaiMatrix4x4(aibone->mOffsetMatrix);
			}
		}

		aiNode* rootNode = m_aiScene->mRootNode;
		aiNode* skeletonRootNode = nullptr;
		for (uint32 i = 0; i < rootNode->mNumChildren; ++i)
		{
			aiNode* node = rootNode->mChildren[i];
			if (bonesMap.contains(node->mName.C_Str()))
			{
				skeletonRootNode = node;
				break;
			}
		}

		if (skeletonRootNode == nullptr)
		{
			ATN_CORE_WARN_TAG("AssetManager", "Failed to import skeleton from {}", m_Path);
			return nullptr;
		}

		std::vector<Bone> bones;
		bones.emplace_back();
		BuildBonesHierarchy(skeletonRootNode, bonesMap, Matrix4::Identity(), bones);

		return Skeleton::Create(bones);
	}

	bool AssimpImporter::HasSkeleton() const
	{
		if (!m_aiScene)
			return false;

		for (uint32 i = 0; i < m_aiScene->mNumMeshes; ++i)
		{
			aiMesh* aimesh = m_aiScene->mMeshes[i];

			if (aimesh->HasBones())
				return true;
		}

		return false;
	}

	void AssimpImporter::BuildBonesHierarchy(const aiNode* ainode, const std::unordered_map<String, Matrix4>& bonesMap, const Matrix4& parentTransform, std::vector<Bone>& bones) const
	{
		Matrix4 localTransform = Utils::ConvertaiMatrix4x4(ainode->mTransformation);
		Matrix4 transform = parentTransform * localTransform;

		String nodeName = ainode->mName.C_Str();
		uint32 nodeIndex = bones.size() - 1;

		if (!bonesMap.contains(nodeName))
		{
			if (ainode->mNumChildren != 0)
			{
				ATN_CORE_ASSERT(ainode->mNumChildren == 1);
				BuildBonesHierarchy(ainode->mChildren[0], bonesMap, transform, bones);
			}
			else
			{
				bones.pop_back();
			}
			return;
		}

		Bone& bone = bones.back();

		if (!bone.IsRoot())
			bones[bone.Parent].Children.push_back(nodeIndex);

		bone.Name = nodeName;
		bone.Index = nodeIndex;
		bone.OffsetMatrix = bonesMap.at(nodeName);
		bone.Children.reserve(ainode->mNumChildren);

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			Bone& child = bones.emplace_back();
			child.Parent = nodeIndex;
			child.Index = bones.size() - 1;

			BuildBonesHierarchy(ainode->mChildren[i], bonesMap, transform, bones);
		}
	}

	void AssimpImporter::TraverseNodes(const Ref<MeshSource>& meshSource, const aiNode* ainode, const Matrix4& parentTransform) const
	{
		Matrix4 localTransform = Utils::ConvertaiMatrix4x4(ainode->mTransformation);
		Matrix4 transform = parentTransform * localTransform;

		MeshNode& meshNode = meshSource->m_Nodes.back();
		uint32 nodeIndex = meshSource->m_Nodes.size() - 1;
		
		if(!meshNode.IsRoot())
			meshSource->m_Nodes[meshNode.Parent].Children.push_back(nodeIndex);

		meshNode.Index = nodeIndex;
		meshNode.Name = ainode->mName.C_Str();
		meshNode.LocalTransform = localTransform;
		meshNode.Children.reserve(ainode->mNumChildren);
		meshNode.SubMeshes.reserve(ainode->mNumMeshes);

		for (uint32 i = 0; i < ainode->mNumMeshes; ++i)
		{
			uint32 meshIndex = ainode->mMeshes[i];
			SubMesh& subMesh = meshSource->m_SubMeshes[meshIndex];

			subMesh.NodeName = meshNode.Name;
			subMesh.LocalTransform = localTransform;
			subMesh.Transform = transform;
			
			meshNode.SubMeshes.push_back(meshIndex);
		}

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			if (ainode->mChildren[i]->mNumMeshes != 0)
			{
				MeshNode& child = meshSource->m_Nodes.emplace_back();
				child.Parent = nodeIndex;

				TraverseNodes(meshSource, ainode->mChildren[i], transform);
			}
		}
	}

	void AssimpImporter::LoadGeometry(const Ref<MeshSource>& meshSource) const
	{
		std::vector<MeshVertex> vertices;
		std::vector<BoneInfluenceVertex> boneInfluenceVertices;
		std::vector<uint32> indices;

		Ref<Skeleton> skeleton = meshSource->m_Skeleton;
		meshSource->m_SubMeshes.reserve(m_aiScene->mNumMeshes);

		for (uint32 i = 0; i < m_aiScene->mNumMeshes; ++i)
		{
			aiMesh* aimesh = m_aiScene->mMeshes[i];
			aiMaterial* aimaterial = m_aiScene->mMaterials[aimesh->mMaterialIndex];

			bool isRigged = meshSource->IsRigged();
			ATN_CORE_ASSERT(isRigged == aimesh->HasBones());

			SubMesh& subMesh = meshSource->m_SubMeshes.emplace_back();
			subMesh.AABB = AABB(Utils::ConvertaiVector3D(aimesh->mAABB.mMin), Utils::ConvertaiVector3D(aimesh->mAABB.mMax));
			subMesh.Name = aimesh->mName.C_Str();
			subMesh.MaterialName = aimaterial->GetName().C_Str();

			uint32 numVertices = aimesh->mNumVertices;
			uint32 numIndices = aimesh->mNumFaces * 3;

			subMesh.BaseVertex = vertices.size();
			subMesh.VertexCount = numVertices;
			subMesh.BaseIndex = indices.size();
			subMesh.IndexCount = numIndices;

			boneInfluenceVertices.reserve(numVertices);
			vertices.reserve(numVertices);
			for (uint32 i = 0; i < numVertices; ++i)
			{
				MeshVertex& vertex = vertices.emplace_back();

				if (aimesh->HasPositions())
				{
					vertex.Position = Utils::ConvertaiVector3D(aimesh->mVertices[i]);
				}

				for (uint32 j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j)
				{
					if (aimesh->HasTextureCoords(j))
					{
						vertex.TexCoords.x = aimesh->mTextureCoords[j][i].x;
						vertex.TexCoords.y = aimesh->mTextureCoords[j][i].y;
						break;
					}
				}

				if (aimesh->HasNormals())
				{
					vertex.Normal = Utils::ConvertaiVector3D(aimesh->mNormals[i]);
				}

				if (aimesh->HasTangentsAndBitangents())
				{
					vertex.Tangent = Utils::ConvertaiVector3D(aimesh->mTangents[i]);
					vertex.Bitangent = Utils::ConvertaiVector3D(aimesh->mBitangents[i]);
				}

				if (isRigged)
					boneInfluenceVertices.emplace_back();
			}

			uint32 numFaces = aimesh->mNumFaces;
			aiFace* faces = aimesh->mFaces;

			indices.reserve(numIndices);
			for (uint32 i = 0; i < numFaces; i++)
			{
				indices.push_back(faces[i].mIndices[0]);
				indices.push_back(faces[i].mIndices[1]);
				indices.push_back(faces[i].mIndices[2]);
			}

			if (!aimesh->HasBones())
			{
				ATN_CORE_ASSERT(!meshSource->IsRigged());
				continue;
			}

			boneInfluenceVertices.resize(vertices.size());
			for (uint32 i = 0; i < aimesh->mNumBones; ++i)
			{
				aiBone* aibone = aimesh->mBones[i];
				uint32 boneID = skeleton->GetBoneIndex(aibone->mName.C_Str());

				for (uint32 j = 0; j < aibone->mNumWeights; ++j)
				{
					uint32 vertexID = subMesh.BaseVertex + aibone->mWeights[j].mVertexId;
					float weight = aibone->mWeights[j].mWeight;

					for (uint32 k = 0; k < ShaderDef::MAX_NUM_BONES_PER_VERTEX; ++k)
					{
						if (boneInfluenceVertices[vertexID].Weights[k] == 0.f)
						{
							boneInfluenceVertices[vertexID].BoneIDs[k] = boneID;
							boneInfluenceVertices[vertexID].Weights[k] = weight;
							break;
						}
						else if (k == ShaderDef::MAX_NUM_BONES_PER_VERTEX - 1)
						{
							ATN_CORE_WARN_TAG("StaticMesh", "Vertex has more than four bones/weights affecting it, extra data will be dicarded(BoneID = {}, Weight = {})",
								boneID, weight);
						}
					}
				}
			}
		}

		VertexBufferCreateInfo vertexBufferInfo;
		vertexBufferInfo.Name = fmt::format("{}_VertexBuffer", m_Path.filename());
		vertexBufferInfo.Data = vertices.data();
		vertexBufferInfo.Size = vertices.size() * sizeof(MeshVertex);
		vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

		meshSource->m_VertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		if (!indices.empty())
		{
			IndexBufferCreateInfo indexBufferInfo;
			indexBufferInfo.Name = fmt::format("{}_IndexBuffer", m_Path.filename());
			indexBufferInfo.Data = indices.data();
			indexBufferInfo.Count = indices.size();
			indexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			meshSource->m_IndexBuffer = IndexBuffer::Create(indexBufferInfo);
		}

		if (meshSource->IsRigged())
		{
			VertexBufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.Name = fmt::format("{}_BonesVertexBuffer", m_Path.filename());
			vertexBufferInfo.Data = boneInfluenceVertices.data();
			vertexBufferInfo.Size = boneInfluenceVertices.size() * sizeof(BoneInfluenceVertex);
			vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			meshSource->m_BonesInfluenceBuffer = VertexBuffer::Create(vertexBufferInfo);
		}
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
