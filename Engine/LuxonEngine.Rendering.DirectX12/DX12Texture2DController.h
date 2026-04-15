#pragma once
#include "BasicTypes.h"
#include <map>
#include "Rendering/GPUTexture2DController.h"

using namespace Microsoft::WRL;

namespace QuantumEngine {
	class Texture2D;
	enum class TextureFormat;
}

namespace QuantumEngine::Rendering::DX12 {
	class DX12Texture2DController : public GPUTexture2DController
	{
	public:
		DX12Texture2DController(const ref<Texture2D>& texture);
		bool Initialize(const ComPtr<ID3D12Device10>& device);
		void UploadToGPU(ComPtr<ID3D12GraphicsCommandList7>& uploadCommandList, const ComPtr<ID3D12Resource2>& uploadTextureBuffer);
		ID3D12DescriptorHeap* GetShaderView() { return m_textureHeap; }
		void Release();
	private:
		ref<Texture2D> m_texture;
		DXGI_FORMAT m_dxFormat;
		ID3D12Resource2* m_tectureResource;
		const static std::map<TextureFormat, DXGI_FORMAT> m_texFormatMaps;
		ID3D12DescriptorHeap* m_textureHeap;
	};
}
