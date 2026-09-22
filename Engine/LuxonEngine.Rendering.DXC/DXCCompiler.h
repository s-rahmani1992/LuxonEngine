#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <wrl/client.h>

#include <string>
#include <vector>

struct IDxcBlob;
struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace LuxonEngine::Rendering::DXC {
	struct DXCCompileOptions {
		std::wstring entryPoint;			
		std::wstring targetProfile;			
		std::vector<std::wstring> includeDirs;
		std::vector<std::wstring> defines;	
		std::vector<std::wstring> arguments;
		bool outputReflection = false;		
	};

	class DXCCompiler
	{
	public:

		DXCCompiler();
		~DXCCompiler();

		DXCCompiler(const DXCCompiler&) = delete;
		DXCCompiler& operator=(const DXCCompiler&) = delete;

		bool Initialize(std::string& error);
		bool Compile(const void* source, size_t size, const DXCCompileOptions& options,
			Microsoft::WRL::ComPtr<IDxcBlob>& outObject, std::string& error,
			Microsoft::WRL::ComPtr<IDxcBlob>* outReflection = nullptr);

		HRESULT CreateReflection(IDxcBlob* reflectionBlob, REFIID riid, void** ppvReflection);

	private:
		Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
		Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_includeHandler;
	};
}
