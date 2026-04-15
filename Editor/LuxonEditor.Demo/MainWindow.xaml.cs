using System.Windows;

namespace LuxonEditor.Demo
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            viewHost.Content = new Views.IntroductionView();
        }

        private void OnSimpleSceneClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.SimpleSceneInfoView();
        }

        private void OnReflectionSceneClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.ReflectionSceneInfoView();
        }

        private void OnShadowSceneClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.ShadowSceneInfoView();
        }

        private void OnRefractionSceneClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.RefractionSceneInfoView();
        }

        private void OnCompleteSceneClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.CompleteSceneInfoView();
        }

        private void OnIntroductionClicked(object sender, RoutedEventArgs e)
        {
            viewHost.Content = new Views.IntroductionView();
        }
    }
}