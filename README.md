# LuxonEngine

## Table of Contents

- [Overview](#overview)
- [Main Features](#main-features)
- [How to set up the project (Windows Only)](#how-to-set-up-the-project-windows-only)
- [Building And Running the Project](#building-and-running-the-project)

## Overview

**LuxonEngine** is a custom render engine for Windows OS capable of rendering scenes using either **rasterization** or **ray tracing**. The engine supports both **DirectX 12** and **Vulkan** as graphics APIs.

> **Note:** Ray tracing requires a GPU with hardware‑accelerated ray tracing support (NVIDIA RTX, AMD RDNA2+, Intel Arc). Running ray‑tracing mode on unsupported hardware may cause crashes.


## Main Features

### Rendering
- Full Ray Tracing Renderer.
- Hybrid Renderer supporting rasterization, compute and ray tracing.
- Mesh, G-Buffer, and Spline renderer components.
- Fixed or unlimited FPS modes.
- In-editor preview renderer. (it only renders meshes with a fixed material at the moment)

### Graphics APIs and Shaders
- DirectX 12 and Vulkan support.
- Macro-driven DXC shader compiler for HLSL targeting both APIs.
- Rasterization, ray tracing, and compute shader support.
- Runtime-editable material instances.

### Asset Import and Management
- Model import (.fbx and .obj) via Assimp (static meshes).
- Texture import (.png , .jpg and .jpeg) via Windows Imaging Component.
- Asset browser with basic file management actions like delete, move and rename.
- Isolated mesh viewer window.

### Serialization
- Materials and scenes are saved and loaded as JSON.
- Boost used for GUIDs and JSON serialization.

### Scene and Entities
- Transform and optional Renderer and Ray Tracing components per entity.
- Directional and point lights.
- Behaviour objects for custom scene logic.

### Editor and Tools
- Qt Widgets based Editor GUI for scene, asset management, material, and shader workflows.
- In-editor shader authoring and compilation.
- Built-in logging console.

### Misc
- Engine API separated from Editor GUI.
- Drag/drop and file dialog import for external files.


## How to set up the project (Windows Only)

### Prerequisites
- Visual Studio 2022 (Desktop development with C++)
- Qt 6 (msvc2022_64) with Widgets
- Vulkan SDK (LunarG)
- CMake (if used)
- Latest GPU drivers (NVIDIA/AMD/Intel)

### 1. Clone
```
git clone https://github.com/s-rahmani1992/LuxonEngine.git
```
### 2. Install Vulkan SDK
Download from https://vulkan.lunarg.com/sdk/home#windows and install.

### 3. Set environment variables
Set `VULKAN_SDK` to your Vulkan SDK path, e.g.:
`C:\VulkanSDK\1.3.xxx.x`
Add `C:\VulkanSDK\1.3.xxx.x\Bin` to PATH if needed.

### 4. Install Qt 6 and Qt Visual Studio Tools
Install Qt 6 (Widgets) matching MSVC toolset and the "Qt Visual Studio Tools" extension for Your Visual studio.

### 5. Set the correct startup project
Open the `.sln` in Visual Studio, set `Editor/LuxonEditor.GUI.Qt` as Startup Project

## Building And Running the Project
Follow the steps below to build **LuxonEngine** from fully setup project.

### 1. Build the project
- choose `x64` and `Release`
- Select Debug->Start Debugging or Start Without Debugging

After the build, the .exe file along with other Assets and DLLs will be at  
(Solution Directory)/Build/Release/

### 2. Run windeployqt
The built project is still not ready because the required Qt DLLs must be added to the build folder. In order to do it:

1. Locate the `windeployqt.exe`. it is located inside the Qt installation folder at the address below
```
C:\Qt\6.x.x\msvc2022_64\bin\windeployqt.exe
```
2. Open the Cmd at the path and run the command with the format below to add required files to your build folder
```
windeployqt.exe "Build Location" --release
```

You can now run LuxonEngine.exe to start the app

