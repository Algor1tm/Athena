#pragma once

#include "Athena/Core/Core.h"


namespace Athena
{
	class VulkanPlatform
	{
	public:
		static std::vector<const char*> GetRequiredInstanceExtensions();
		static std::vector<const char*> GetRequiredDeviceExtensions();
	};
}
