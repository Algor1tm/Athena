#pragma once

#include "Athena/Renderer/Shader.h"
#include "Athena/Renderer/Buffer.h"

#include "D3D11GraphicsContext.h"


namespace Athena
{
	class ATHENA_API D3D11Shader : public Shader
	{
	public:
		D3D11Shader(const BufferLayout& layout, const Filepath& filepath);
		D3D11Shader(const BufferLayout& layout, const String& name, const String& vertexSrc, const String& fragmentSrc);
		virtual ~D3D11Shader();

		virtual void Bind() const override;
		virtual void UnBind() const override;

		virtual void Reload() override;

	private:
		void Compile(const std::unordered_map<ShaderType, String>& shaderSources);
		void CreateInputLayout(const BufferLayout& layout);

	private:
		std::unordered_map<ShaderType, Microsoft::WRL::ComPtr<ID3DBlob>> m_ShaderByteCodes;
		Microsoft::WRL::ComPtr<ID3D11VertexShader> m_VertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> m_PixelShader;

		Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	};
}
