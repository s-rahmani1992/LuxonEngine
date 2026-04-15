// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include <Platform/CommonWin.h>
#include <d3d12.h>
#include <dxcapi.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#ifdef _DEBUG
#include <d3d12sdklayers.h>
#include <dxgidebug.h>
#endif

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "libboost_json-vc143-mt-gd-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-gd-x64-1_90.lib")
#else
#pragma comment(lib, "libboost_json-vc143-mt-x64-1_90.lib")
#pragma comment(lib, "libboost_container-vc143-mt-x64-1_90.lib")
#endif

//d3d12.lib, d3dcompiler.lib, dxgi.lib, and d3d12.lib

#endif //PCH_H
