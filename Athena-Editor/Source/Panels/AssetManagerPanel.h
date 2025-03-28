#pragma once

#include "Athena/Core/Core.h"
#include "Panels/Panel.h"


namespace Athena
{
	class AssetManagerPanel : public Panel
	{
	public:
		AssetManagerPanel(const Ref<EditorContext>& context);
		virtual void OnImGuiRender() override;

	private:
		bool Filter(AssetHandle handle, const AssetMetadata& metadata);

	private:
		String m_SearchString;
	};
}
