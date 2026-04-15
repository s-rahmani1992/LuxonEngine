using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;

namespace LuxonEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for ShadowSceneInfoView.xaml
    /// </summary>
    public partial class ShadowSceneInfoView : UserControl
    {
        public ShadowSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnShadowRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunShadowScene(interop.Handle, GraphicAPI.DIRECTX_12);
        }

        private void OnShadowRayTracingVK(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunShadowScene(interop.Handle, GraphicAPI.VULKAN);
        }
    }
}
