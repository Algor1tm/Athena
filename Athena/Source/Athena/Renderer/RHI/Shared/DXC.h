#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Core/Buffer.h"

#if ATN_PLATFORM_WINDOWS
	#include <Windows.h>
#endif

#include <dxcapi.h>

namespace Athena::DXC
{
	class Compiler
	{
	public:
		Compiler();
		~Compiler();

		NON_COPYABLE_NON_MOVABLE(Compiler);

		void SetDebugName(const String& name) { m_Name = name; }
		void SetIncludeDirectories(const std::vector<FilePath>& dirs);

		bool Preprocess(const String& source, const std::vector<std::wstring>& args, String* result);
		bool Compile(const String& source, const std::vector<std::wstring>& args, IDxcBlob** reflection, Buffer* result);

		IDxcUtils* GetUtils() const { return m_DxcUtils; }

	private:
		bool CompileInternal(const String& source, const std::vector<std::wstring>& args, IDxcResult** dxcResult);

	private:
		IDxcUtils* m_DxcUtils;
		IDxcCompiler3* m_DxcCompiler;
		IDxcIncludeHandler* m_IncludeHandler;
		String m_Name;
	};
}
