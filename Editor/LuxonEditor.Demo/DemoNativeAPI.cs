using System.Runtime.InteropServices;

namespace LuxonEditor.Demo
{
    enum GraphicAPI
    {
        DIRECTX_12,
        VULKAN,
    }

    enum RenderMode
    {
        HYBRID,
        RAY_TRACING,
    }

    internal static partial class DemoNativeAPI
    {
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Simple_Scene")]
        public static extern bool RunSimpleScene(IntPtr handle, GraphicAPI graphicAPI, RenderMode renderMode);


        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Reflection_Scene")]
        public static extern bool RunReflectionScene(IntPtr handle, GraphicAPI graphicAPI, RenderMode renderMode);

       
        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Shadow_Scene")]
        public static extern bool RunShadowScene(IntPtr handle, GraphicAPI graphicAPI);


        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Refraction_Scene")]
        public static extern bool RunRefractionScene(IntPtr handle, GraphicAPI graphicAPI);


        [DllImport("QuantumEngine.DemoAPI.dll", EntryPoint = "Run_Complete_Scene")]
        public static extern bool RunCompleteScene(IntPtr handle, GraphicAPI graphicAPI);
    }
}
