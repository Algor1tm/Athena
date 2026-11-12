#include "DXC.h"
#include "Athena/Platform/Windows/WindowsDebug.h"


namespace Athena::DXC
{
	class FileSystemIncludeHandler final : public IDxcIncludeHandler
	{
	public:
		FileSystemIncludeHandler(IDxcUtils* utils, const std::vector<FilePath>& includeDirectories)
			: m_Utils(utils)
			, m_IncludeDirectories(includeDirectories)
		{
			m_Utils->AddRef();
		}

		~FileSystemIncludeHandler()
		{
			m_Utils->Release();
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
		{
			if (!ppvObject)
				return E_POINTER;

			*ppvObject = nullptr;

			if (riid == IID_IUnknown || riid == __uuidof(IDxcIncludeHandler))
			{
				*ppvObject = static_cast<FileSystemIncludeHandler*>(this);
				AddRef();
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		ULONG STDMETHODCALLTYPE AddRef() override
		{
			return ++m_RefCount;
		}

		ULONG STDMETHODCALLTYPE Release() override
		{
			ULONG refCount = --m_RefCount;

			if (refCount == 0)
				delete this;

			return refCount;
		}

		HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
		{
			if (!pFilename || !ppIncludeSource)
				return E_INVALIDARG;

			*ppIncludeSource = nullptr;

			FilePath filename(pFilename);

			// Try the filename directly.
			HRESULT hr = LoadFile(filename, ppIncludeSource);

			if (SUCCEEDED(hr))
				return hr;

			// Try configured include directories.
			for (const auto& directory : m_IncludeDirectories)
			{
				hr = LoadFile(directory / filename, ppIncludeSource);

				if (SUCCEEDED(hr))
					return hr;
			}

			return hr;
		}

	private:
		HRESULT LoadFile(const std::filesystem::path& path, IDxcBlob** ppBlob)
		{
			IDxcBlobEncoding* blob = nullptr;
			HRESULT hr = m_Utils->LoadFile(path.c_str(), nullptr, &blob);

			if (FAILED(hr))
				return hr;

			*ppBlob = blob;
			return S_OK;
		}

	private:
		IDxcUtils* m_Utils = nullptr;
		std::atomic<ULONG> m_RefCount = 1;
		std::vector<std::filesystem::path> m_IncludeDirectories;
	};


	Compiler::Compiler()
	{
		CHECK_HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_DxcUtils)));
		CHECK_HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_DxcCompiler)));
		m_IncludeHandler = nullptr;
	}

	Compiler::~Compiler()
	{
		if (m_IncludeHandler)
			m_IncludeHandler->Release();

		m_DxcUtils->Release();
		m_DxcCompiler->Release();
	}

	void Compiler::SetIncludeDirectories(const std::vector<FilePath>& dirs)
	{
		if (m_IncludeHandler)
			m_IncludeHandler->Release();

		m_IncludeHandler = new FileSystemIncludeHandler(m_DxcUtils, dirs);
	}

	bool Compiler::Preprocess(const String& source, const std::vector<std::wstring>& args, String* resultSource)
	{
		IDxcResult* result = nullptr;
		bool compileResult = CompileInternal(source, args, &result);

		if (!compileResult || !result->HasOutput(DXC_OUT_HLSL))
		{
			result->Release();
			return false;
		}

		IDxcBlobUtf8* blob;
		CHECK_HR(result->GetOutput(DXC_OUT_HLSL, IID_PPV_ARGS(&blob), nullptr));

		*resultSource = String(blob->GetStringPointer(), blob->GetStringLength());

		blob->Release();
		result->Release();

		return true;
	}

	bool Compiler::Compile(const String& source, const std::vector<std::wstring>& args, IDxcBlob** reflection, Buffer* resultBytecode)
	{
		IDxcResult* result = nullptr;
		bool compileResult = CompileInternal(source, args, &result);

		if (!compileResult || !result->HasOutput(DXC_OUT_OBJECT))
		{
			result->Release();
			return false;
		}

		IDxcBlob* blob;
		CHECK_HR(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr));

		*resultBytecode = Buffer::Copy(blob->GetBufferPointer(), blob->GetBufferSize());

		if (reflection)
		{
			if(!result->HasOutput(DXC_OUT_REFLECTION))
			{
				result->Release();
				return false;
			}

			CHECK_HR(result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(reflection), nullptr));
		}

		blob->Release();
		result->Release();

		return true;
	}

	bool Compiler::CompileInternal(const String& source, const std::vector<std::wstring>& args, IDxcResult** dxcResult)
	{
		std::vector<LPCWSTR> arguments;
		arguments.reserve(args.size());

		for (const auto& arg : args)
		{
			arguments.push_back(arg.c_str());
		}

		DxcBuffer sourceBuffer = {};
		sourceBuffer.Ptr = source.data();
		sourceBuffer.Size = source.size();
		sourceBuffer.Encoding = DXC_CP_UTF8;

		CHECK_HR(m_DxcCompiler->Compile(&sourceBuffer, arguments.data(), arguments.size(), m_IncludeHandler, IID_PPV_ARGS(dxcResult)));

		HRESULT status;
		(*dxcResult)->GetStatus(&status);

		if (FAILED(status))
		{
			IDxcBlobUtf8* errors;
			CHECK_HR((*dxcResult)->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));

			String errorsString = String(errors->GetStringPointer(), errors->GetStringLength());
			ATN_LOG_ERROR(Renderer, "Failed to compile to shader '{}', errors: {}", m_Name, errorsString);
			ensuref(false);

			errors->Release();
			return false;
		}
		else
		{
			IDxcBlobUtf8* warnings = nullptr;
			CHECK_HR((*dxcResult)->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&warnings), nullptr));

			if (warnings && warnings->GetStringLength() > 0)
			{
				String warningsString = String(warnings->GetStringPointer(), warnings->GetStringLength());
				ATN_LOG_WARN(Renderer, "Shader '{}' had warnings during compilation, warnings: {}", m_Name, warningsString);
				ensuref(false);
			}

			warnings->Release();
		}

		return true;
	}
}
