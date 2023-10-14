#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	enum class ShaderType
	{
		VERTEX_SHADER = 0,
		FRAGMENT_SHADER = 1,
		GEOMETRY_SHADER = 2,
		COMPUTE_SHADER = 3
	};

	class ATHENA_API Shader
	{
	public:
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const FilePath& path, const String& name);

		virtual ~Shader() = default;

		virtual bool IsCompiled() = 0;
		virtual void Reload() = 0;

		const String& GetName() const { return m_Name; }

	protected:
		String m_Name;
		FilePath m_FilePath;
	};

	class ATHENA_API ShaderLibrary
	{
	public:
		void Add(const String& name, const Ref<Shader>& shader);

		Ref<Shader> Load(const String& name, const FilePath& path);
		Ref<Shader> Get(const String& name);

		bool Exists(const String& name);
		void Reload();

	private:
		std::unordered_map<String, Ref<Shader>> m_Shaders;
	};
}
