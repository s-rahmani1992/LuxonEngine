#include "DXCCompiler.h"
#include <dxcapi.h>

using Microsoft::WRL::ComPtr;

LuxonEngine::Rendering::DXC::DXCCompiler::DXCCompiler() = default;

LuxonEngine::Rendering::DXC::DXCCompiler::~DXCCompiler() = default;

bool LuxonEngine::Rendering::DXC::DXCCompiler::Initialize(std::string& error)
{
	if (FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)))) {
		error = "Failed to create DXC utils";
		return false;
	}

	if (FAILED(m_utils->CreateDefaultIncludeHandler(&m_includeHandler))) {
		error = "Failed to create DXC include handler";
		return false;
	}

	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		error = "Failed to create DXC compiler";
		return false;
	}

	return true;
}

bool LuxonEngine::Rendering::DXC::DXCCompiler::Compile(const void* source, size_t size, const DXCCompileOptions& options, ComPtr<IDxcBlob>& outObject, std::string& error, ComPtr<IDxcBlob>* outReflection)
{
	if (m_compiler == nullptr) {
		error = "DXC compiler is not initialized";
		return false;
	}

	DxcBuffer sourceBuffer{
		.Ptr = source,
		.Size = size,
		.Encoding = DXC_CP_ACP,
	};

	std::vector<LPCWSTR> arguments;
	arguments.reserve(4 + 2 * (options.includeDirs.size() + options.defines.size()) + options.arguments.size());

	if (!options.entryPoint.empty()) {
		arguments.push_back(L"-E");
		arguments.push_back(options.entryPoint.c_str());
	}

	arguments.push_back(L"-T");
	arguments.push_back(options.targetProfile.c_str());

	for (const auto& includeDir : options.includeDirs) {
		arguments.push_back(L"-I");
		arguments.push_back(includeDir.c_str());
	}

	for (const auto& define : options.defines) {
		arguments.push_back(L"-D");
		arguments.push_back(define.c_str());
	}

	for (const auto& argument : options.arguments)
		arguments.push_back(argument.c_str());

	ComPtr<IDxcResult> compileResult;
	HRESULT result;
	result = m_compiler->Compile(&sourceBuffer, arguments.data(), (UINT32)arguments.size(), m_includeHandler.Get(), IID_PPV_ARGS(&compileResult));

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return false;
	}

	ComPtr<IDxcBlobUtf8> pErrors;

	result = compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.GetAddressOf()), nullptr);

	if (FAILED(result)) {
		error = "Unknown Error when Beginning to compile";
		return false;
	}

	if (pErrors && pErrors->GetStringLength() > 0)
	{
		error = std::string(pErrors->GetStringPointer(), pErrors->GetStringLength());
		return false;
	}

	result = compileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&outObject), nullptr);

	if (FAILED(result) || outObject == nullptr) {
		error = "Unknown Error when Obtaining Shader Bytecode";
		return false;
	}

	if (options.outputReflection && outReflection != nullptr) {
		result = compileResult->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(outReflection->GetAddressOf()), nullptr);

		if (FAILED(result) || *outReflection == nullptr) {
			error = "Unknown Error when Obtaining Reflection Bytecode";
			return false;
		}
	}

	return true;
}

HRESULT LuxonEngine::Rendering::DXC::DXCCompiler::CreateReflection(IDxcBlob* reflectionBlob, REFIID riid, void** ppvReflection)
{
	if (m_utils == nullptr || reflectionBlob == nullptr)
		return E_POINTER;

	DxcBuffer reflectionBuffer{
		.Ptr = reflectionBlob->GetBufferPointer(),
		.Size = reflectionBlob->GetBufferSize(),
		.Encoding = 0,
	};

	return m_utils->CreateReflection(&reflectionBuffer, riid, ppvReflection);
}
