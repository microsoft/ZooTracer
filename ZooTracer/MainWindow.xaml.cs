using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace ZooTracer
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public static RoutedCommand SaveTraceCommand = new RoutedUICommand("Save the trace", "SaveTrace", typeof(MainWindow));
        public static RoutedCommand LoadTraceCommand = new RoutedUICommand("Load a trace", "LoadTrace", typeof(MainWindow));
        public static RoutedCommand StartHereCommand = new RoutedUICommand("Start trace from current frame", "StartHere", typeof(MainWindow));
        public static RoutedCommand StopHereCommand = new RoutedUICommand("End trace at current frame", "StopHere", typeof(MainWindow));
        public MainWindow()
        {
            InitializeComponent();
            var w = System.Windows.SystemParameters.MaximizedPrimaryScreenWidth;
            var h = System.Windows.SystemParameters.MaximizedPrimaryScreenHeight;
            if (w < 1366 || h < 768) WindowState = WindowState.Maximized;
        }

        ZooTracerModel getModel() { return FindResource("ZooTracerModel") as ZooTracerModel; }

        private void Open_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            e.CanExecute = true;
        }

        private void Open_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.Filter = "Video Files|*.avi;*.wmv;*.mp4;*.mpeg;*.mkv|All files (*.*)|*.*";
            dlg.Title = "open video";
            var result = dlg.ShowDialog();
            if (result == true)
                Properties.Settings.Default.videofilepath = dlg.FileName;
        }
        private void Close_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            var model = getModel();
            e.CanExecute = model != null && model.IsOpen;
            e.Handled = true;
        }
        private void Close_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            Properties.Settings.Default.videofilepath = "";
        }
        private void Left_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            var model = getModel();
            e.CanExecute = model != null && model.IsOpen && model.CurrentFrameNumber > 0;
            e.Handled = true;
        }
        private void Left_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var model = getModel();
            if (model != null && model.IsOpen && model.CurrentFrameNumber > 0) model.CurrentFrameNumber -= 1;
        }
        private void Right_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            var model = getModel();
            e.CanExecute = model != null && model.IsOpen && model.CurrentFrameNumber < model.MaxFrameNumber;
            e.Handled = true;
        }
        private void Right_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var model = getModel();
            if (model != null && model.IsOpen && model.CurrentFrameNumber < model.MaxFrameNumber) model.CurrentFrameNumber += 1;
        }
        private void SaveTrace_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            //var model = getModel();
            e.CanExecute = true;// model != null && model.IsOpen;
            e.Handled = true;
        }
        private void SaveTrace_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.SaveFileDialog();
            dlg.Filter = "CSV Files|*.csv";
            dlg.Title = "save the trace as";
            var result = dlg.ShowDialog();
            if (result == true)
            {
                var model = getModel();
                model.SaveTrace(dlg.FileName);
            }
        }
        private void LoadTrace_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            //var model = getModel();
            e.CanExecute = true;// model != null && model.IsOpen;
            e.Handled = true;
        }
        private void LoadTrace_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.Filter = "CSV Files|*.csv";
            dlg.Title = "open a trace";
            var result = dlg.ShowDialog();
            if (result == true)
            {
                var model = getModel();
                model.LoadTrace(dlg.FileName);
            }
        }
        private void StartHere_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            var model = getModel();
            e.CanExecute = (model != null) && model.CanTrace && model.IsBeforeTraceEnd();
            e.Handled = true;
        }
        private void StartHere_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var model = getModel();
            if (model != null) model.StartHere();
        }
        private void StopHere_CanExecute(object sender, CanExecuteRoutedEventArgs e)
        {
            var model = getModel();
            e.CanExecute = (model != null) && model.CanTrace && model.IsAfterTraceStart();
            e.Handled = true;
        }
        private void StopHere_Executed(object sender, ExecutedRoutedEventArgs e)
        {
            var model = getModel();
            if (model != null) model.StopHere();
        }

        private void videoFrame_MouseMove(object sender, MouseEventArgs e)
        {
            var vf = sender as Image;
            if (vf != null)
            {
                var p = e.GetPosition(vf);
                if (vf.Stretch == Stretch.Uniform) // the only stretch type supported
                {
                    // The following is in measure units (96ths of an inch). We always create images with 96dpi resolution, so it's in pixels.
                    var fw = vf.Source.Width;
                    var fh = vf.Source.Height;
                    var iw = vf.ActualWidth;
                    var ih = vf.ActualHeight;
                    // scale the position to frame pixel coords
                    double scale = Math.Min(iw / fw, ih / fh); // image = scale*frame
                    int x = (int)(0.5 * fw + (p.X - 0.5 * iw) / scale);
                    int y = (int)(0.5 * fh + (p.Y - 0.5 * ih) / scale);
                    var model = getModel();
                    if (model != null) model.CursorPosition = Tuple.Create(x, y);
                    e.Handled = true;
                }
                else throw new NotImplementedException("vf doesn't support Stretch." + vf.Stretch.ToString());
            }
        }

        private void videoFrame_MouseLeave(object sender, MouseEventArgs e)
        {
            var model = getModel();
            if (model != null) model.CursorPosition = null;
        }

        private void glass_Loaded(object sender, RoutedEventArgs e)
        {
            var model = getModel();
            if (model != null)
            {
                model.MakeGlassItems();
            }
            updateGlassSize(sender, e);
        }
        private void updateGlassSize(object sender, RoutedEventArgs e)
        {
            var glass = sender as FrameworkElement;
            var model = getModel();
            if (glass != null && model != null)
            {
                model.GlassHeight = glass.ActualHeight;
                model.GlassWidth = glass.ActualWidth;
            }
        }

        private void videoFrame_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton == MouseButton.Left)
            {
                var model = getModel();
                if (model != null)
                    model.SetKeyPoint();
                e.Handled = true;
            }
        }

        private void videoFrame_TouchMove(object sender, TouchEventArgs e)
        {
            var vf = sender as Image;
            if (vf != null)
            {
                var p = e.GetTouchPoint(vf);
                if (vf.Stretch == Stretch.Uniform) // the only stretch type supported
                {
                    // The following is in measure units (96ths of an inch). We always create images with 96dpi resolution, so it's in pixels.
                    var fw = vf.Source.Width;
                    var fh = vf.Source.Height;
                    var iw = vf.ActualWidth;
                    var ih = vf.ActualHeight;
                    // scale the position to frame pixel coords
                    double scale = Math.Min(iw / fw, ih / fh); // image = scale*frame
                    int x = (int)(0.5 * fw + (p.Position.X - 0.5 * iw) / scale);
                    int y = (int)(0.5 * fh + (p.Position.Y - 0.5 * ih) / scale);
                    var model = getModel();
                    if (model != null) model.CursorPosition = Tuple.Create(x, y);
                    e.Handled = true;
                }
                else throw new NotImplementedException("vf doesn't support Stretch." + vf.Stretch.ToString());
            }

        }

        private void videoFrame_TouchLeave(object sender, TouchEventArgs e)
        {
            var model = getModel();
            if (model != null) model.CursorPosition = null;
        }

        private void videoFrame_TouchUp(object sender, TouchEventArgs e)
        {
            var model = getModel();
            if (model != null)
                model.SetKeyPoint();
            e.Handled = true;
        }

        private void Occlude_Click(object sender, RoutedEventArgs e)
        {
            var model = getModel();
            if (model != null) model.SetOccludedFrame();
        }
        private void Clear_Click(object sender, RoutedEventArgs e)
        {
            var model = getModel();
            if (model != null) model.SetAutoFrame();
        }
        private void ClearAll_Click(object sender, RoutedEventArgs e)
        {
            var model = getModel();
            if (model != null) model.ClearAll();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            Properties.Settings.Default.Save();
        }

        private void Hyperlink_Click(object sender, RoutedEventArgs e)
        {
            var hl = sender as Hyperlink;
            if (hl != null)
            {
                var t = hl.Inlines.FirstInline as Run;
                if (t != null && t.Text.Length > 0 && new Uri(t.Text).Scheme == Uri.UriSchemeHttp) Process.Start(Uri.EscapeUriString(t.Text));
            }
        }
    }
}
