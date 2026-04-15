#pragma once

#include "../BasicTypes.h"

namespace QuantumEngine::Rendering {
	class GPUTexture2DController;
}

namespace QuantumEngine {
	enum class TextureFormat { //TODO Add More Formats
		Unknown,
		RGBA32,
		BGRA32,
	};

	struct TextureProperties {
		Byte* data;
		bool copyPixelData;
		UInt32 width;
		UInt32 height;
		UInt32 size;
		UInt32 bpp;
		UInt32 channelCount;
		TextureFormat format;
	};

	class Texture2D {
	public:
		Texture2D(const TextureProperties& properties);
		inline UInt32 GetWidth() const { return m_width; }
		inline UInt32 GetHeight() const { return m_height; }
		inline UInt32 GetTotalSize() const { return m_size; }
		inline TextureFormat GetFormat() const { return m_format; }
		inline UInt32 GetBytePerPixel() const { return (m_bpp + 7) / 8; }
		Byte* GetData() { return m_data; }
		ref<Rendering::GPUTexture2DController> GetGPUHandle() { return m_gpuHandle; }
		void SetGPUHandle(ref<Rendering::GPUTexture2DController> handle) { m_gpuHandle = handle; }
		void Release();
	private:
		Byte* m_data;
		UInt32 m_width;
		UInt32 m_height;
		UInt32 m_size;
		UInt32 m_bpp;
		TextureFormat m_format;
		ref<Rendering::GPUTexture2DController> m_gpuHandle;
	};
}