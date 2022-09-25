#include "atnpch.h"
#include "D3D11Shader.h"

#include <d3dcompiler.h>


namespace Athena
{
	static DXGI_FORMAT ShaderDataTypeToDXGIFormat(ShaderDataType type, bool normalized)
	{
		switch (type)
		{
		case ShaderDataType::Float:  return DXGI_FORMAT_R32_FLOAT;
		case ShaderDataType::Float2: return DXGI_FORMAT_R32G32_FLOAT;
		case ShaderDataType::Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
		case ShaderDataType::Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case ShaderDataType::Int:    return !normalized ? DXGI_FORMAT_R32_SINT : DXGI_FORMAT_R16_SNORM;
		case ShaderDataType::Int2:   return !normalized ? DXGI_FORMAT_R32G32_SINT : DXGI_FORMAT_R16G16_SNORM;
		case ShaderDataType::Int3:   return !normalized ? DXGI_FORMAT_R32G32B32_SINT : DXGI_FORMAT_R8G8B8A8_SNORM;
		case ShaderDataType::Int4:   return !normalized ? DXGI_FORMAT_R32G32B32A32_SINT : DXGI_FORMAT_R16G16B16A16_SNORM;
		case ShaderDataType::Bool:   return DXGI_FORMAT_R8_UINT;
		}

		ATN_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return DXGI_FORMAT();
	}

	static std::string_view ShaderTypeToD3DVersion(ShaderType type)
	{
		if (type == ShaderType::VERTEX_SHADER) return "vs_5_0";
		if (type == ShaderType::FRAGMENT_SHADER) return "ps_5_0";

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return "";
	}

	static std::string_view ShaderTypeToEntryPoint(ShaderType type)
	{
		if (type == ShaderType::VERTEX_SHADER) return "VSMain";
		if (type == ShaderType::FRAGMENT_SHADER) return "PSMain";

		ATN_CORE_ASSERT(false, "Unknown shader type!");
		return "";
	}


	D3D11Shader::D3D11Shader(const BufferLayout& layout, const Filepath& filepath)
	{
		ATN_CORE_ASSERT(std::filesystem::exists(filepath), "Invalid filepath for Shader");
		const auto& strFilepath = filepath.string();

		SetNameFromFilepath(strFilepath);

		String result = ReadFile(strFilepath);
		auto shaderSources = PreProcess(result);
		Compile(shaderSources);

		CreateInputLayout(layout);
		m_ShaderByteCodes.clear();
	}

	D3D11Shader::D3D11Shader(const BufferLayout& layout, const String& name, const String& vertexSrc, const String& fragmentSrc)
	{
		m_Name = name;

		std::unordered_map<ShaderType, String> sources;
		sources[ShaderType::VERTEX_SHADER] = vertexSrc;
		sources[ShaderType::FRAGMENT_SHADER] = fragmentSrc;
		Compile(sources);

		CreateInputLayout(layout);
		m_ShaderByteCodes.clear();
	}

	D3D11Shader::~D3D11Shader()
	{

	}

	void D3D11Shader::Compile(const std::unordered_map<ShaderType, String>& shaderSources)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> errorMsg;

		for (auto&& [type, sourceString] : shaderSources)
		{
			HRESULT hr = D3DCompile(
				shaderSources.at(type).c_str(),
				shaderSources.at(type).length(),
				nullptr,
				nullptr,
				nullptr,
				ShaderTypeToEntryPoint(type).data(),
				ShaderTypeToD3DVersion(type).data(),
				D3DCOMPILE_ENABLE_STRICTNESS, 
				0,
				m_ShaderByteCodes[type].GetAddressOf(),
				errorMsg.GetAddressOf());

			if (errorMsg != nullptr)
			{
				ATN_CORE_WARN("Shader '{0}':\n{1}", m_Name, (char*)errorMsg->GetBufferPointer());
			}
			ATN_CORE_ASSERT(SUCCEEDED(hr), "Shader Compilation Failure!");
			
			if (type == ShaderType::VERTEX_SHADER)
				hr = D3D11CurrentContext::Device->CreateVertexShader(m_ShaderByteCodes.at(type)->GetBufferPointer(), m_ShaderByteCodes.at(type)->GetBufferSize(), nullptr, m_VertexShader.GetAddressOf());
			else if (type == ShaderType::FRAGMENT_SHADER)
				hr = D3D11CurrentContext::Device->CreatePixelShader(m_ShaderByteCodes.at(type)->GetBufferPointer(), m_ShaderByteCodes.at(type)->GetBufferSize(), nullptr, m_PixelShader.GetAddressOf());
			else
				ATN_CORE_ASSERT(false, "Invalid Shader Type!");

			ATN_CORE_ASSERT(SUCCEEDED(hr), "Failed to Create DirectX Shader!");
		}
	}


	void D3D11Shader::Bind() const
	{
		D3D11CurrentContext::DeviceContext->IASetInputLayout(m_InputLayout.Get());

		D3D11CurrentContext::DeviceContext->VSSetShader(m_VertexShader.Get(), nullptr, 0);
		D3D11CurrentContext::DeviceContext->PSSetShader(m_PixelShader.Get(), nullptr, 0);
	}

	void D3D11Shader::UnBind() const
	{
		D3D11CurrentContext::DeviceContext->IASetInputLayout(nullptr);

		D3D11CurrentContext::DeviceContext->VSSetShader(nullptr, nullptr, 0);
		D3D11CurrentContext::DeviceContext->PSSetShader(nullptr, nullptr, 0);
	}

	void D3D11Shader::CreateInputLayout(const BufferLayout& layout)
	{
		std::vector<D3D11_INPUT_ELEMENT_DESC> d3dLayoutDesc;
		d3dLayoutDesc.reserve(layout.GetElements().size());
		for (const auto& elem : layout.GetElements())
		{
			if (elem.Type == ShaderDataType::Mat3)
			{
				for (SIZE_T i = 0; i < 3; ++i)
				{
					D3D11_INPUT_ELEMENT_DESC desc;
					desc.SemanticName = elem.Name.c_str();
					desc.SemanticIndex = (UINT)i;
					desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
					desc.InputSlot = 0;
					desc.AlignedByteOffset = elem.Offset + (UINT)i * 12;
					desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					desc.InstanceDataStepRate = 0;

					d3dLayoutDesc.push_back(desc);
				}
			}
			else if (elem.Type == ShaderDataType::Mat4)
			{
				for (SIZE_T i = 0; i < 4; ++i)
				{
					D3D11_INPUT_ELEMENT_DESC desc;
					desc.SemanticName = elem.Name.c_str();
					desc.SemanticIndex = (UINT)i;
					desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					desc.InputSlot = 0;
					desc.AlignedByteOffset = elem.Offset + (UINT)i * 16;
					desc.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
					desc.InstanceDataStepRate = 0;

					d3dLayoutDesc.push_back(desc);
				}
			}
			else
			{
				D3D11_INPUT_ELEMENT_DESC desc;
				desc.SemanticName = elem.Name.c_str();
				desc.SemanticIndex = 0;
				desc.Format = ShaderDataTypeToDXGIFormat(elem.Type, elem.Normalized);
				desc.InputSlot = 0;
				desc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;//elem.Offset;
				desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
				desc.InstanceDataStepRate = 0;

				d3dLayoutDesc.push_back(desc);
			}
		}

		D3D11CurrentContext::Device->CreateInputLayout(
			d3dLayoutDesc.data(), 
			(UINT)d3dLayoutDesc.size(), 
			m_ShaderByteCodes[ShaderType::VERTEX_SHADER]->GetBufferPointer(), 
			m_ShaderByteCodes[ShaderType::VERTEX_SHADER]->GetBufferSize(),
			m_InputLayout.GetAddressOf());
	}

	void D3D11Shader::SetInt(const String& name, int value)
	{

	}

	void D3D11Shader::SetIntArray(const String& name, int* value, uint32 count)
	{

	}

	void D3D11Shader::SetFloat(const String& name, float value)
	{

	}

	void D3D11Shader::SetFloat3(const String& name, const Vector3& vec3)
	{

	}

	void D3D11Shader::SetFloat4(const String& name, const Vector4& vec4)
	{

	}

	void D3D11Shader::SetMat4(const String& name, const Matrix4& mat4)
	{

	}
}
