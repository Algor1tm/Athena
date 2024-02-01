#pragma once

#include "Athena/Core/Core.h"

#include "Athena/Renderer/Color.h"
#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/GPUBuffers.h"


namespace Athena
{
	class ATHENA_API Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const String& name);
		static Ref<Material> CreatePBRStatic(const String& name);
		virtual ~Material();

		virtual void Set(std::string_view name, const Ref<ShaderResource>& resource) = 0;

		virtual void Set(std::string_view name, const Matrix4& value) = 0;
		virtual void Set(std::string_view name, const Vector4& value) = 0;
		virtual void Set(std::string_view name, float value) = 0;
		virtual void Set(std::string_view name, uint32 value) = 0;

		virtual void Bind() = 0;
		virtual void RT_UpdateForRendering() = 0;

		Ref<Shader> GetShader() { return m_Shader; }
		const String& GetName() { return m_Name; }

	protected:
		Ref<Shader> m_Shader;
		String m_Name;
	};


	class ATHENA_API MaterialTable : public RefCounted
	{
	public:
		Ref<Material> GetMaterial(const String& name) const;
		void AddMaterial(const Ref<Material>& material);
		void RemoveMaterial(const Ref<Material>& material);

		bool Exists(const String& name) const;

	private:
		std::unordered_map<String, Ref<Material>> m_Materials;
	};
}
