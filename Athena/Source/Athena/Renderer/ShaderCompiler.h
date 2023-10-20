#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"

#include <unordered_map>


namespace Athena
{
	using ShaderSources = std::unordered_map<ShaderStage, String>;
	using ShaderBinaries = std::unordered_map<ShaderStage, std::vector<uint32>>;

	class ATHENA_API ShaderCompiler
	{
	public:
		ShaderCompiler(const FilePath& filepath, const String& name);

		bool CompileOrGetFromCache(bool forceCompile = false);

		bool IsHlsl() const { return m_IsHlsl; }
		std::string_view GetEntryPoint(ShaderStage stage) const;
		const ShaderBinaries& GetBinaries() const { return m_SPIRVBinaries; }

	private:
		bool CompileAndWriteToCache(const ShaderSources& sources);
		void ReadFromCache();

		void Reflect(ShaderStage type, const std::vector<uint32>& src);
		bool PreProcess(ShaderSources& shaderSources);
		bool CheckShaderStages(const ShaderSources& sources);
		bool Parse(const String& source, ShaderSources& result);

	private:
		FilePath m_FilePath;
		String m_Name;
		bool m_IsHlsl;
		std::unordered_map<ShaderStage, FilePath> m_CachedFilePaths;
		bool m_Recompile;
		ShaderBinaries m_SPIRVBinaries;
	};

}
