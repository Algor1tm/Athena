#include "MeshImporter.h"

#include "Athena/Asset/AssetManager.h"
#include "Athena/Asset/Editor/TextureImporter.h"
#include "Athena/Core/FileSystem.h"
#include "Athena/Core/YAMLTypes.h"
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
		aiProcess_OptimizeMeshes |

		aiProcess_Triangulate |
		aiProcess_FlipUVs;

	MeshImporter::MeshImporter(const Ref<MeshImportSettings>& settings)
	{
		m_Settings = settings;
	}

	bool MeshImporter::ImportToMesh(const FilePath& path, WeakRef<Mesh> mesh)
	{
		m_Path = path;

		unsigned int importFlags = s_ImportFlags;

		if (m_Settings->CollapseGraph)
			importFlags |= aiProcess_OptimizeGraph;

		m_aiScene = aiImportFile(m_Path.string().c_str(), importFlags);

		if (m_aiScene == nullptr)
		{
			const char* error = aiGetErrorString();
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to import mesh from {}.", m_Path);
			ATN_CORE_ERROR("	Assimp Error: {}", error);
			return false;
		}

		if (HasSkeleton() && m_Settings->ImportAnimations)
		{
			Ref<Skeleton> skeleton = ImportSkeleton();

			if (skeleton)
			{
				mesh->m_IsRigged = true;
				mesh->m_Skeleton = skeleton;

				mesh->m_Animations.reserve(m_aiScene->mNumAnimations);
				for (uint32 i = 0; i < m_aiScene->mNumAnimations; ++i)
				{
					Ref<Animation> animation = ImportAnimation(i, skeleton);
					mesh->m_Animations.push_back(animation);
				}
			}
		}
		else
		{
			mesh->m_IsRigged = false;
			mesh->m_Skeleton = nullptr;
			mesh->m_Animations.clear();
		}

		mesh->m_MaterialTable.clear();
		MaterialTable& matTable = mesh->m_MaterialTable;
		for (uint32 i = 0; i < m_aiScene->mNumMaterials; ++i)
		{
			aiMaterial* aimaterial = m_aiScene->mMaterials[i];
			String name = aimaterial->GetName().C_Str();
			
			if (m_Settings->OverrideMaterials.contains(name))
			{
				matTable[name] = m_Settings->OverrideMaterials.at(name);
			}
			else
			{
				matTable[name] = LoadMaterial(aimaterial);
			}
		}

		mesh->m_SubMeshes.clear();
		LoadGeometry(mesh, m_Settings->SubMeshIndices);

		mesh->m_Nodes.clear();
		mesh->m_Nodes.emplace_back();

		TraverseNodes(mesh, FindRootNode(m_aiScene->mRootNode));

		//mesh->m_Nodes[0].Name = m_Path.stem().string();

#if 0
		ATN_CORE_WARN("Node hierarchy for mesh {}", m_Path);
		Utils::PrintNodes(m_aiScene->mRootNode);
#endif

		aiReleaseImport(m_aiScene);

		return true;
	}

	Ref<Animation> MeshImporter::ImportAnimation(uint32 animationIndex, const Ref<Skeleton>& skeleton) const
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

	Ref<Skeleton> MeshImporter::ImportSkeleton() const
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

	bool MeshImporter::HasSkeleton() const
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

	void MeshImporter::BuildBonesHierarchy(const aiNode* ainode, const std::unordered_map<String, Matrix4>& bonesMap, const Matrix4& parentTransform, std::vector<Bone>& bones) const
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

	const aiNode* MeshImporter::FindRootNode(const aiNode* root)
	{
		while (root)
		{
			if (root->mNumMeshes > 0)
				return root;

			if (root->mNumChildren != 1)
				return root;

			root = root->mChildren[0];
		}

		return root;
	}

	void MeshImporter::TraverseNodes(const Ref<Mesh>& mesh, const aiNode* ainode, const Matrix4& parentTransform) const
	{
		Matrix4 localTransform = Utils::ConvertaiMatrix4x4(ainode->mTransformation);
		Matrix4 transform = parentTransform * localTransform;

		MeshNode& meshNode = mesh->m_Nodes.back();
		uint32 nodeIndex = mesh->m_Nodes.size() - 1;
		
		if(!meshNode.IsRoot())
			mesh->m_Nodes[meshNode.Parent].Children.push_back(nodeIndex);

		meshNode.Index = nodeIndex;
		meshNode.Name = ainode->mName.C_Str();
		meshNode.LocalTransform = localTransform;
		meshNode.Children.reserve(ainode->mNumChildren);
		meshNode.SubMeshes.reserve(ainode->mNumMeshes);

		for (uint32 i = 0; i < ainode->mNumMeshes; ++i)
		{
			uint32 meshIndex = ainode->mMeshes[i];
			SubMesh& subMesh = mesh->m_SubMeshes[meshIndex];

			subMesh.NodeName = meshNode.Name;
			subMesh.LocalTransform = localTransform;
			subMesh.Transform = transform;
			
			meshNode.SubMeshes.push_back(meshIndex);
		}

		for (uint32 i = 0; i < ainode->mNumChildren; ++i)
		{
			if (ainode->mChildren[i]->mNumMeshes != 0)
			{
				MeshNode& child = mesh->m_Nodes.emplace_back();
				child.Parent = nodeIndex;

				TraverseNodes(mesh, ainode->mChildren[i], transform);
			}
		}
	}

	void MeshImporter::LoadGeometry(const Ref<Mesh>& mesh, const std::vector<uint32>& subMeshIndices) const
	{
		std::vector<MeshVertex> vertices;
		std::vector<BoneInfluenceVertex> boneInfluenceVertices;
		std::vector<uint32> indices;

		float scale = m_Settings->Scale;

		Ref<Skeleton> skeleton = mesh->m_Skeleton;
		mesh->m_SubMeshes.reserve(m_aiScene->mNumMeshes);

		for (uint32 i = 0; i < m_aiScene->mNumMeshes; ++i)
		{
			if (!subMeshIndices.empty())
			{
				auto it = std::find(subMeshIndices.begin(), subMeshIndices.end(), i);
				if (it == subMeshIndices.end())
					continue;
			}
			
			aiMesh* aimesh = m_aiScene->mMeshes[i];
			aiMaterial* aimaterial = m_aiScene->mMaterials[aimesh->mMaterialIndex];

			bool isRigged = mesh->IsRigged();
			ATN_CORE_ASSERT(isRigged == aimesh->HasBones());

			SubMesh& subMesh = mesh->m_SubMeshes.emplace_back();
			subMesh.AABB = AABB(scale * Utils::ConvertaiVector3D(aimesh->mAABB.mMin), scale * Utils::ConvertaiVector3D(aimesh->mAABB.mMax));
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
					vertex.Position = scale * Utils::ConvertaiVector3D(aimesh->mVertices[i]);
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
				ATN_CORE_ASSERT(!mesh->IsRigged());
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

		mesh->m_VertexBuffer = VertexBuffer::Create(vertexBufferInfo);

		if (!indices.empty())
		{
			IndexBufferCreateInfo indexBufferInfo;
			indexBufferInfo.Name = fmt::format("{}_IndexBuffer", m_Path.filename());
			indexBufferInfo.Data = indices.data();
			indexBufferInfo.Count = indices.size();
			indexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			mesh->m_IndexBuffer = IndexBuffer::Create(indexBufferInfo);
		}

		if (mesh->IsRigged())
		{
			VertexBufferCreateInfo vertexBufferInfo;
			vertexBufferInfo.Name = fmt::format("{}_BonesVertexBuffer", m_Path.filename());
			vertexBufferInfo.Data = boneInfluenceVertices.data();
			vertexBufferInfo.Size = boneInfluenceVertices.size() * sizeof(BoneInfluenceVertex);
			vertexBufferInfo.Flags = BufferMemoryFlags::GPU_ONLY;

			mesh->m_BonesInfluenceBuffer = VertexBuffer::Create(vertexBufferInfo);
		}
	}

	AssetHandle MeshImporter::LoadMaterial(const aiMaterial* aimaterial) const
	{
		Ref<MaterialAsset> material = Ref<MaterialAsset>::Create();
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

	AssetHandle MeshImporter::LoadMaterialTexture(const aiMaterial* aimaterial, uint32 type, bool srgb) const
	{
		AssetHandle handle = 0;

		aiString texFilepath;
		if (AI_SUCCESS == aimaterial->Get(AI_MATKEY_TEXTURE(type, 0), texFilepath))
		{
			const aiTexture* embeddedTex = m_aiScene->GetEmbeddedTexture(texFilepath.C_Str());
			if (embeddedTex)
			{
				void* data = embeddedTex->pcData;
				uint32 width = embeddedTex->mWidth;
				uint32 height = embeddedTex->mHeight;

				Ref<TextureImportSettings> importSettings = Ref<TextureImportSettings>::Create();
				importSettings->Name = String(texFilepath.C_Str(), texFilepath.length);
				importSettings->sRGB = srgb;
				importSettings->GenerateMipMaps = true;
				importSettings->FilterMode = TextureFilter::TRILINEAR;
				importSettings->WrapMode = TextureWrap::REPEAT;
				importSettings->AnisotropyLevel = 8.f;

				TextureImporter importer(importSettings);

				Ref<Texture2D> texture = importer.ImportFromMemory(data, width, height);
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


	Ref<AssetImportSettings> MeshImportSettings::Clone() const
	{
		Ref<MeshImportSettings> cloneSettings = Ref<MeshImportSettings>::Create();

		cloneSettings->ImportAnimations = ImportAnimations;
		cloneSettings->CollapseGraph = CollapseGraph;
		cloneSettings->Scale = Scale;
		cloneSettings->SubMeshIndices = SubMeshIndices;
		cloneSettings->OverrideMaterials = OverrideMaterials;

		return cloneSettings;
	}

	bool MeshImportSettings::Serialize(const FilePath& absolutePath) const
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "MeshImportSettings" << YAML::Value << YAML::BeginMap;

		out << YAML::Key << "ImportAnimations" << YAML::Value << ImportAnimations;
		out << YAML::Key << "CollapseGraph" << YAML::Value << CollapseGraph;
		out << YAML::Key << "Scale" << YAML::Value << Scale;
		out << YAML::Key << "SubMeshIndices" << YAML::Value << SubMeshIndices;
		out << YAML::Key << "OverrideMaterials" << YAML::Value << YAML::BeginMap;
		for (const auto& [name, handle] : OverrideMaterials)
		{
			if (AssetManager::IsAssetHandleValid(handle) && !AssetManager::GetAssetMetadata(handle).IsMemoryOnly)
				out << YAML::Key << name << YAML::Value << handle;
		}
		out << YAML::EndMap;

		out << YAML::EndMap;
		out << YAML::EndMap;

		std::ofstream fout(absolutePath);
		fout << out.c_str();

		return true;
	}

	bool MeshImportSettings::Deserialize(const FilePath& absolutePath)
	{
		YAML::Node data = YAML::TryLoadYAMLFile(absolutePath);
		if (!data)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to mesh import settings from {}!", absolutePath);
			return false;
		}

		YAML::Node root = data["MeshImportSettings"];
		if (!root)
		{
			ATN_CORE_ERROR_TAG("AssetManager", "Failed to mesh import settings from {}!", absolutePath);
			return false;
		}

		ImportAnimations = TryReadYAMLValue<bool>(root, "ImportAnimations", true);
		CollapseGraph = TryReadYAMLValue<bool>(root, "CollapseGraph", true);
		Scale = TryReadYAMLValue<float>(root, "Scale", 1.f);
		SubMeshIndices = TryReadYAMLValue<std::vector<uint32>>(root, "SubMeshIndices", std::vector<uint32>());

		OverrideMaterials.clear();
		YAML::Node materialsNode = root["OverrideMaterials"];
		for (const auto& it : materialsNode)
		{
			OverrideMaterials.insert({ it.first.as<String>(), it.second.as<AssetHandle>() });
		}

		if (ImportAnimations == true && CollapseGraph == false)
		{
			ATN_CORE_WARN_TAG("AssetManager", "Forcing CollapseGraph to true in MeshImportSettings");
			CollapseGraph = true;
		}
		
		if (Scale <= 0.f)
			Scale = 1.f;
	
		return true;
	}
}
