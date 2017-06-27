using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace ZooTracer
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            if (!String.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("OpenCVDir")))
            {
                var platform = Environment.Is64BitProcess ? "x64" : "x86";
                Environment.SetEnvironmentVariable("PATH",
                    Environment.ExpandEnvironmentVariables(@"%PATH%;%OpenCVDir%" + platform + @"\vc11\bin"));
            }
            base.OnStartup(e);
        }
        private void Application_Startup(object sender, StartupEventArgs e)
        {
            if (e != null && e.Args != null && e.Args.Length > 0)
            {
                if (e.Args.Length == 1 && e.Args[0] == "build_pca") {
                    ZooTracerModel.BuildProjector();
                }
                else
                    MessageBox.Show("The only command line parameter accepted is 'build_pca'");
                Shutdown();
            }


            ((ZooTracerModel)FindResource("ZooTracerModel")).LoadSettings();
        }
    }
}
