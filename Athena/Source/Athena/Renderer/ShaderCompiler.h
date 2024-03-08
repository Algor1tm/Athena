#pragma once

#include "Athena/Core/Core.h"
#include "Athena/Renderer/Shader.h"

#include <unordered_map>


namespace Athena
{
	using ShaderBinaries = std::unordered_map<ShaderStage, std::vector<uint32>>;

	enum class ShaderLanguage
	{
		NONE,
		GLSL,
		HLSL
	};


	class ATHENA_API ShaderCompiler
	{
	public:
		ShaderCompiler(const FilePath& filepath, const String& name);

		bool CompileOrGetFromCache(bool forceCompile = false);

		std::string_view GetEntryPoint(ShaderStage stage) const;
		const ShaderBinaries& GetBinaries() const { return m_SPIRVBinaries; }

		ShaderMetaData Reflect();

	private:
		struct StageDescription
		{
			ShaderStage Stage;
			FilePath FilePathToCache;
			String Source;
		};

		struct PreProcessResult
		{
			std::vector<StageDescription> StageDescriptions;
			bool ParseResult;
			bool NeedRecompile;
		};

	private:
		bool CompileAndWriteToCache(const PreProcessResult& result);
		void ReadFromCache(const PreProcessResult& result);

		PreProcessResult PreProcess();
		std::vector<StageDescription> PreProcessShaderStages();
		std::vector<StageDescription> GetHLSLStageDescriptions();
		std::vector<StageDescription> GetGLSLStageDescriptions();

		void GetLanguageAndEntryPoints();

	private:
		FilePath m_FilePath;
		String m_Name;
		ShaderLanguage m_Language;
		ShaderBinaries m_SPIRVBinaries;

		std::unordered_map<ShaderStage, std::string_view> m_StageToEntryPointMap;
	};
}
