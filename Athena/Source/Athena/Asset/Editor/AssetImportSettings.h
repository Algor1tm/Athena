#pragma once

#include "Athena/Core/Core.h"

#include <unordered_map>


namespace Athena
{
	class ATHENA_API AssetImportSettings : public RefCounted
	{
	public:
		virtual ~AssetImportSettings() = default;
		virtual Ref<AssetImportSettings> Clone() const = 0;

		virtual bool Serialize(const FilePath& absolutePath) const = 0;
		virtual bool Deserialize(const FilePath& absolutePath) = 0;
	};
}
