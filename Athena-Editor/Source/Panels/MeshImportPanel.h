#pragma once

#include "Athena/Core/Core.h"
#include "Panels/Panel.h"


namespace Athena
{
	class MeshImportPanel : public Panel
	{
	public:
		MeshImportPanel(const Ref<EditorContext>& context);
		virtual void OnImGuiRender() override;

		void OnImport(AssetHandle meshSourceHandle);

		void CreateStaticMesh(AssetHandle meshHandle);
		void CreateSkeletalMesh(AssetHandle meshHandle);

	private:
		void OnClose();
		void CreateEntityHierarchy(const Ref<MeshSource>& meshSource, const MeshNode& meshNode, Entity entity, AssetHandle meshHandle);

	private:
		String m_StaticMeshExt, m_SkeletalMeshExt;
		AssetHandle m_MeshSourceHandle = 0;

		bool m_IsStaticMesh = true;
		bool m_ImportAnimations = false;
		bool m_IsRigged = false;
		FilePath m_FilePath;
	};
}
