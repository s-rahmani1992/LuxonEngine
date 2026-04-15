using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace LuxonEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for SimpleSceneInfoView.xaml
    /// </summary>
    public partial class SimpleSceneInfoView : UserControl
    {
        public SimpleSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnSimpleRasterizationDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleScene(interop.Handle, GraphicAPI.DIRECTX_12, RenderMode.HYBRID);
        }

        private void OnSimpleRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleScene(interop.Handle, GraphicAPI.DIRECTX_12, RenderMode.RAY_TRACING);
        }

        private void OnSimpleRasterizationVK(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleScene(interop.Handle, GraphicAPI.VULKAN, RenderMode.HYBRID);
        }

        private void OnSimpleRayTracingVK(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunSimpleScene(interop.Handle, GraphicAPI.VULKAN, RenderMode.RAY_TRACING);
        }
    }
}
