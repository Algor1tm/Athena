#pragma once

#include "Athena/Math/Vector.h"
#include "Athena/Math/Matrix.h"

#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	enum class ShaderStage
	{
		VERTEX_STAGE = 0,
		FRAGMENT_STAGE = 1,
		GEOMETRY_STAGE = 2,
		COMPUTE_STAGE = 3
	};

	class ATHENA_API Shader : public RefCounted
	{
	public:
		static Ref<Shader> Create(const FilePath& path);
		static Ref<Shader> Create(const FilePath& path, const String& name);

		virtual ~Shader() = default;

		bool IsCompiled() const { return m_IsCompiled; }
		virtual void Reload() = 0;

		const String& GetName() const { return m_Name; }

	protected:
		String m_Name;
		FilePath m_FilePath;
		bool m_IsCompiled;
	};

	class ATHENA_API ShaderPack : public RefCounted
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
