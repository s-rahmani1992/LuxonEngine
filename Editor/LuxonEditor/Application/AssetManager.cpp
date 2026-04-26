#include "AssetManager.h"
#include <wrl/client.h>
#include <StringUtilities.h>
#include "EditorApplication.h"

using namespace Microsoft::WRL;

#pragma comment(lib, "WindowsCodecs.lib")

LuxonEditor::AssetManager::AssetManager(ID3D11Device* device)
	:m_device(device)
{
	auto result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_wicImageFactory));
	
	std::string path = EditorApplication::GetProjectPath();

	ImportIcon(path + "\\Contents\\icons\\folder.png", FOLDER_ICON_ID);
	ImportIcon(path + "\\Contents\\icons\\texture.png", TEXTURE_ICON_ID);
	ImportIcon(path + "\\Contents\\icons\\hlsl.png", HLSL_ICON_ID);
	ImportIcon(path + "\\Contents\\icons\\default.png", DEFAULT_ICON_ID);
	ImportIcon(path + "\\Contents\\icons\\3d-model.png", MODEL_ICON_ID);
}

LuxonEditor::AssetManager::~AssetManager()
{
	
}

void LuxonEditor::AssetManager::ImportIcon(const std::string& filePath, const std::string& idName)
{
	ComPtr<IWICFormatConverter> m_wicConverter;
	m_wicImageFactory->CreateFormatConverter(&m_wicConverter);

	ComPtr<IWICStream> wicFileStream;
	if (FAILED(m_wicImageFactory->CreateStream(&wicFileStream))) {
		return;
	}

	if (FAILED(wicFileStream->InitializeFromFilename(CharToString(filePath.c_str()).c_str(), GENERIC_READ))) {
		return;
	}

	ComPtr<IWICBitmapDecoder> wicBitmapDecoder;
	if (FAILED(m_wicImageFactory->CreateDecoderFromStream(wicFileStream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &wicBitmapDecoder))) {
		return;
	}

	ComPtr<IWICBitmapFrameDecode> wicBitmapFrameDecode;

	if (FAILED(wicBitmapDecoder->GetFrame(0, &wicBitmapFrameDecode))) {
		return;
	}

	IWICBitmapSource* targetBitmapSource = wicBitmapFrameDecode.Get();

	WICPixelFormatGUID origPixelFormat;
	UINT width, height;
	wicBitmapFrameDecode->GetSize(&width, &height);
	wicBitmapFrameDecode->GetPixelFormat(&origPixelFormat);
	WICPixelFormatGUID finalPixelFormat = origPixelFormat;

	if (origPixelFormat != GUID_WICPixelFormat32bppRGBA) { // we need to convert image to format compatible with DXGI_FORMAT
		finalPixelFormat = GUID_WICPixelFormat32bppRGBA;

		HRESULT r = m_wicConverter->Initialize(wicBitmapFrameDecode.Get(), finalPixelFormat, WICBitmapDitherTypeErrorDiffusion, 0, 0, WICBitmapPaletteTypeCustom);

		if (FAILED(r)) {
			return;
		}

		targetBitmapSource = m_wicConverter.Get();
	}

	ComPtr<IWICComponentInfo> wicComInfo;
	if (FAILED(m_wicImageFactory->CreateComponentInfo(finalPixelFormat, &wicComInfo))) {
		return;
	}

	ComPtr<IWICPixelFormatInfo> wicPixelFormatInfo;
	if (FAILED(wicComInfo->QueryInterface(IID_PPV_ARGS(&wicPixelFormatInfo)))) {
		return;
	}

	UINT bpp, channelCount;
	wicPixelFormatInfo->GetBitsPerPixel(&bpp);
	wicPixelFormatInfo->GetChannelCount(&channelCount);

	UINT32 stride = ((bpp + 7) / 8) * width;
	UINT32 size = stride * height;

	auto data = new byte[size];

	WICRect rect{
		.X = 0,
		.Y = 0,
		.Width = (INT)width,
		.Height = (INT)height,
	};

	targetBitmapSource->CopyPixels(&rect, stride, size, data);

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA subResource = {};
	subResource.pSysMem = data;
	subResource.SysMemPitch = width * 4;

	ID3D11Texture2D* texture = nullptr;
	m_device->CreateTexture2D(&desc, &subResource, &texture);
	ID3D11ShaderResourceView* textureView = nullptr;
	m_device->CreateShaderResourceView(texture, nullptr, &textureView);
	texture->Release(); // SRV holds a reference
	delete[] data;
	targetBitmapSource->Release();
	targetBitmapSource = nullptr;
	m_icons.emplace(idName, (ImTextureID)textureView);
}

ImTextureID LuxonEditor::AssetManager::GetIcon(const std::string& idName)
{
	auto it = m_icons.find(idName);

	if (it != m_icons.end())
		return (*it).second;

	return m_icons[DEFAULT_ICON_ID];
}

ImTextureID LuxonEditor::AssetManager::GetIconFromExtention(const std::string& extName)
{
	if (extName == ".hlsl" || extName == ".hlsli")
		return GetIcon(HLSL_ICON_ID);
	if (extName == ".obj" || extName == ".fbx")
		return GetIcon(MODEL_ICON_ID);
	if (extName == ".png" || extName == ".jpeg" || extName == ".jpg")
		return GetIcon(TEXTURE_ICON_ID);

	return m_icons[DEFAULT_ICON_ID];
}
