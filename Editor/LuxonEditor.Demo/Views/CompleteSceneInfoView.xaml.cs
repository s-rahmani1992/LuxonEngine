using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace LuxonEditor.Demo.Views
{
    /// <summary>
    /// Interaction logic for CompleteSceneInfoView.xaml
    /// </summary>
    public partial class CompleteSceneInfoView : UserControl
    {
        public CompleteSceneInfoView()
        {
            InitializeComponent();
        }

        private void OnCompleteRayTracingDX12(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunCompleteScene(interop.Handle, GraphicAPI.DIRECTX_12);
        }

        private void OnCompleteRayTracingVK(object sender, RoutedEventArgs e)
        {
            WindowInteropHelper interop = new(Window.GetWindow(this));
            DemoNativeAPI.RunCompleteScene(interop.Handle, GraphicAPI.VULKAN);
        }
    }
}
