#include "DX12API.h"
#include "DX12GPUDeviceManager.h"

Render::GPUDeviceManager* CreateDX12Application()
{
	auto dx12App = new Render::DX12::DX12GPUDeviceManager();
	dx12App->Initialize();
	return dx12App;
}
