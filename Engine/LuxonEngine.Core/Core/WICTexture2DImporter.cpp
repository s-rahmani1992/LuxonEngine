#include "WICTexture2DImporter.h"
#include <wrl/client.h>

using namespace Microsoft::WRL;

const GuidMap<QuantumEngine::TextureFormat> QuantumEngine::WICTexture2DImporter::m_texFormatMaps{
	{GUID_WICPixelFormat32bppRGBA, QuantumEngine::TextureFormat::RGBA32},
	{GUID_WICPixelFormat32bppBGRA, QuantumEngine::TextureFormat::BGRA32},
};

const GuidMap<GUID> QuantumEngine::WICTexture2DImporter::m_convertFormatMaps{
	{GUID_WICPixelFormat24bppRGB, GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat24bppBGR, GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat48bppRGB, GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat48bppBGR, GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat64bppRGBA, GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat64bppBGRA, GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat16bppGray, GUID_WICPixelFormat32bppRGBA},
};

ref<QuantumEngine::Texture2D> QuantumEngine::WICTexture2DImporter::Import(const std::wstring& filePath, std::string& error)
{
	ComPtr<IWICImagingFactory> wicImageFactory;
	ComPtr<IWICFormatConverter> wicConverter;

	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicImageFactory)))) {
		error = "Failed to create wic image factory";
		return nullptr;
	}

	ComPtr<IWICStream> wicFileStream;
	if (FAILED(wicImageFactory->CreateStream(&wicFileStream))) {
		error = "Failed to create wic stream";
		return nullptr;
	}

	if (FAILED(wicFileStream->InitializeFromFilename(filePath.c_str(), GENERIC_READ))) {
		error = "Failed to initialize stream from file";
		return nullptr;
	}

	ComPtr<IWICBitmapDecoder> wicBitmapDecoder;
	if (FAILED(wicImageFactory->CreateDecoderFromStream(wicFileStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder))) {
		error = "Failed to create bitmap decoder";
		return nullptr;
	}

	ComPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;
	
	if (FAILED(wicBitmapDecoder->GetFrame(0, &wicBitmapFrameDecode))) {
		error = "Failed to get first frame of the image";
		return nullptr;
	}

	IWICBitmapSource* targetBitmapSource = wicBitmapFrameDecode.Get();

	TextureProperties texProperties;
	WICPixelFormatGUID origPixelFormat;

	wicBitmapFrameDecode->GetSize(&texProperties.width, &texProperties.height);
	wicBitmapFrameDecode->GetPixelFormat(&origPixelFormat);
	WICPixelFormatGUID finalPixelFormat = origPixelFormat;

	auto formatIt = m_texFormatMaps.find(origPixelFormat);

	if (formatIt == m_texFormatMaps.end()) { // we need to convert image to format compatible with DXGI_FORMAT
		auto convertibleFormatIt = m_convertFormatMaps.find(origPixelFormat);

		if (convertibleFormatIt == m_convertFormatMaps.end()) {
			error = "Cannot decode the image with format";
			return nullptr;
		}

		finalPixelFormat = (*convertibleFormatIt).second;

		wicImageFactory->CreateFormatConverter(&wicConverter);

		if (FAILED(wicConverter->Initialize(wicBitmapFrameDecode.Get(), finalPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom))) {
			error = "Failed to convert to suitable format";
			return nullptr;
		}

		targetBitmapSource = wicConverter.Get();

		formatIt = m_texFormatMaps.find(finalPixelFormat);
		texProperties.format = (*formatIt).second;
	}
	else
		texProperties.format = (*formatIt).second;

	ComPtr<IWICComponentInfo> wicComInfo;
	if (FAILED(wicImageFactory->CreateComponentInfo(finalPixelFormat, &wicComInfo))) {
		error = "Failed to create component info";
		return nullptr;
	}

	ComPtr<IWICPixelFormatInfo> wicPixelFormatInfo;
	if (FAILED(wicComInfo->QueryInterface(IID_PPV_ARGS(&wicPixelFormatInfo)))) {
		error = "Failed to get pixel format info";
		return nullptr;
	}

	wicPixelFormatInfo->GetBitsPerPixel(&texProperties.bpp);
	wicPixelFormatInfo->GetChannelCount(&texProperties.channelCount);
	
	UInt32 stride = ((texProperties.bpp + 7) / 8) * texProperties.width;
	texProperties.size = stride * texProperties.height;

	texProperties.data = new Byte[texProperties.size];

	WICRect rect{
		.X = 0,
		.Y = 0,
		.Width = (INT)texProperties.width,
		.Height = (INT)texProperties.height,
	};

	texProperties.copyPixelData = false;
	targetBitmapSource->CopyPixels(&rect, stride, texProperties.size, texProperties.data);

	return std::make_shared<Texture2D>(texProperties);
}
