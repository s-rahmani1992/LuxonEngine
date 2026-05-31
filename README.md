# LuxonEngine

**LuxonEngine** is a custom render engine for Windows OS capable of rendering scenes using either **rasterization** or **ray tracing**. The engine supports both **DirectX 12** and **Vulkan** as graphics APIs.

A demo build with several pre‑made scenes is available for download.

> **Note:** Ray tracing requires a GPU with hardware‑accelerated ray tracing support (NVIDIA RTX, AMD RDNA2+, Intel Arc). Running ray‑tracing mode on unsupported hardware may cause crashes.

---

## Main Features
- It features two main renderers: a full ray tracing renderer and a hybrid renderer that can utilize rasterization or a combination of rasterization and ray tracing.
- The engine can use either DirectX 12 or Vulkan API
- It includes a feature for importing and rendering 3D model files using the ‘Assimp’ package ([link](https://assimp.org/)). Currently, it only supports static meshes.
- It offers the capability to import and utilize textures for rendering via the ‘Windows Image Component’ SDK.
- It is capable of compiling and using rasterization, compute and ray tracing HLSL shaders with the ‘DXC Compiler’.
- For every rasterization and ray tracing shader, we can generate material instances with unique parameters and textures for customization.
- Each material’s properties can be modified at runtime.
- The application can render at a fixed frame rate or with unlimited FPS, depending on the settings.
- Each object has a ‘transform’ component for its position and orientation, a ‘Renderer component’ for the Hybrid Renderer, and a ‘Ray Tracing component’ for the Ray Tracing Renderer.
- By holding the right mouse button, the user can navigate through the scene using mouse movement and the WASD keys.
- Applications can include ‘Behaviour’ objects that allow for custom control over scene like object movement, data logging, and more.

---

## Building & Running the Project (Visual Studio)

Follow the steps below to build **LuxonEngine** from source.

### 1. Clone the repository
```bash
git clone https://github.com/s-rahmani1992/LuxonEngine.git
```
### 2. Update your GPU drivers
Install the latest NVIDIA / AMD / Intel drivers just in case.

### 3. Install the Vulkan SDK
If you already installed the Vulkan SDK, skip this step

Download the Windows version of Vulkan from LunarG:
https://vulkan.lunarg.com/sdk/home#windows

### 4. Set the VULKAN_SDK_PATH environment variable
If it does not already exist:
- Name: VULKAN_SDK_PATH
- Value: Path to your Vulkan SDK installation

example
```code
C:\VulkanSDK\1.3.xxx.x
```

### 5. Set the correct startup project
- Open the generated .sln file using Visual Studio.
- Right‑click the Demo Project at 'Editor/LuxonEditor.Demo' which is a WPF project
- Select 'Set as Startup Project'

### 6. Build and run
- Select either Debug or Release Configuration 
- Select Debug->Start Debugging or Start Without Debugging

After the build, the .exe file along with other Assets and DLLs will be at  
(Solution Directory)/Build/(Configuration)/
