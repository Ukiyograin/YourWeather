using System.Windows;
using WeatherApp.Services;
using WeatherApp.ViewModels;

namespace WeatherApp
{
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            
            // 创建服务和视图模型
            var weatherService = new WeatherService();
            var viewModel = new MainViewModel(weatherService);
            
            DataContext = viewModel;
        }
    }
}