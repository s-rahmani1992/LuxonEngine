#include "WICTextureImporter.h"
#include "AssetRegistry.h"
#include <EngineAPI.h>
#include <Core/SerializationStream.h>
#include <Core/Texture2D.h>
#include <Core/GUIDUtility.h>
#include <wrl/client.h>
#include <wincodec.h>
#include <string>
#include "GuidUtilities.h"
#include "EngineApplication.h"

#pragma comment(lib, "WindowsCodecs.lib")

using namespace Microsoft::WRL;

static const GuidMap<LuxonEngine::TextureFormat> s_texFormatMaps{
	{GUID_WICPixelFormat32bppRGBA, LuxonEngine::TextureFormat::RGBA32},
	{GUID_WICPixelFormat32bppBGRA, LuxonEngine::TextureFormat::BGRA32},
};

static const GuidMap<GUID> s_convertFormatMaps{
	{GUID_WICPixelFormat24bppRGB,  GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat24bppBGR,  GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat48bppRGB,  GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat48bppBGR,  GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat64bppRGBA, GUID_WICPixelFormat32bppRGBA},
	{GUID_WICPixelFormat64bppBGRA, GUID_WICPixelFormat32bppBGRA},
	{GUID_WICPixelFormat16bppGray, GUID_WICPixelFormat32bppRGBA},
};

static ref<LuxonEngine::Texture2D> ImportFromWICStream(IWICImagingFactory* wicFactory, IWICStream* wicStream, std::string& error)
{
	ComPtr<IWICBitmapDecoder> wicBitmapDecoder;
	if (FAILED(wicFactory->CreateDecoderFromStream(wicStream, nullptr, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder))) {
		error = "Failed to create bitmap decoder from memory stream";
		return nullptr;
	}

	ComPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;
	if (FAILED(wicBitmapDecoder->GetFrame(0, &wicBitmapFrameDecode))) {
		error = "Failed to get first frame of the image";
		return nullptr;
	}

	IWICBitmapSource* targetBitmapSource = wicBitmapFrameDecode.Get();

	LuxonEngine::TextureProperties texProperties;
	WICPixelFormatGUID origPixelFormat;

	wicBitmapFrameDecode->GetSize(&texProperties.width, &texProperties.height);
	wicBitmapFrameDecode->GetPixelFormat(&origPixelFormat);
	WICPixelFormatGUID finalPixelFormat = origPixelFormat;

	auto formatIt = s_texFormatMaps.find(origPixelFormat);

	ComPtr<IWICFormatConverter> wicConverter;
	if (formatIt == s_texFormatMaps.end()) {
		auto convertibleFormatIt = s_convertFormatMaps.find(origPixelFormat);
		if (convertibleFormatIt == s_convertFormatMaps.end()) {
			error = "Cannot decode the image with the given pixel format";
			return nullptr;
		}

		finalPixelFormat = (*convertibleFormatIt).second;

		wicFactory->CreateFormatConverter(&wicConverter);
		if (FAILED(wicConverter->Initialize(wicBitmapFrameDecode.Get(), finalPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom))) {
			error = "Failed to convert to a suitable pixel format";
			return nullptr;
		}

		targetBitmapSource = wicConverter.Get();
		formatIt = s_texFormatMaps.find(finalPixelFormat);
		texProperties.format = (*formatIt).second;
	}
	else {
		texProperties.format = (*formatIt).second;
	}

	ComPtr<IWICComponentInfo> wicComInfo;
	if (FAILED(wicFactory->CreateComponentInfo(finalPixelFormat, &wicComInfo))) {
		error = "Failed to create WIC component info";
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
	texProperties.copyPixelData = false;

	WICRect rect{
		.X = 0,
		.Y = 0,
		.Width = (INT)texProperties.width,
		.Height = (INT)texProperties.height,
	};

	targetBitmapSource->CopyPixels(&rect, stride, texProperties.size, texProperties.data);

	return std::make_shared<LuxonEngine::Texture2D>(texProperties);
}

ref<LuxonEngine::Texture2D> LuxonEditor::WICTextureImporter::Import(const Byte* data, long size, LuxonEngine::SerializationStream& stream, const std::string& filePath, std::string& error)
{
	ComPtr<IWICImagingFactory> wicFactory;
	if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicFactory)))) {
		error = "Failed to create WIC imaging factory";
		return nullptr;
	}

	ComPtr<IWICStream> wicStream;
	if (FAILED(wicFactory->CreateStream(&wicStream))) {
		error = "Failed to create WIC stream";
		return nullptr;
	}

	if (FAILED(wicStream->InitializeFromMemory(const_cast<BYTE*>(reinterpret_cast<const BYTE*>(data)), static_cast<DWORD>(size)))) {
		error = "Failed to initialize WIC stream from memory";
		return nullptr;
	}

	auto texture = ImportFromWICStream(wicFactory.Get(), wicStream.Get(), error);
	if (!texture)
		return nullptr;

	auto guid = stream.GetGuid("uuid");
	EngineApplication::GetAssetManager()->AddTexture(guid, fs::path(filePath).filename().string(), texture);
	return texture;
}

LuxonEngine::SerializationStream LuxonEditor::WICTextureImporter::GenerateMetaFromFile(const Byte* data, long size)
{
	LuxonEngine::SerializationStream stream;
	stream.SetGuid("uuid", LuxonEditor::GuidGenerator::GenerateGUID());
	return stream;
}
