#pragma once

#include "Athena/Asset/AssetManager.h"
#include "Athena/Core/Core.h"
#include "Panels/Panel.h"
#include "Athena/Renderer/MaterialAsset.h"


namespace Athena
{
	class MaterialEditorPanel : public Panel
	{
	public:
		MaterialEditorPanel(const Ref<EditorContext>& context);
		virtual void OnImGuiRender() override;
		void SetActiveMaterial(AssetHandle material);

	private:
		void RenderTexture(const Ref<MaterialAsset>& material, MaterialTextureType type) const;

	private:
		AssetHandle m_ActiveMaterial;
	};
}
