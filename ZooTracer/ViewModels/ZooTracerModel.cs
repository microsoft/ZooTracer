using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Settings = ZooTracer.Properties.Settings;

namespace ZooTracer
{
    public class ZooTracerModel : System.ComponentModel.INotifyPropertyChanged
    {
        private WriteableBitmap myFrameImageSource;
        private WriteableBitmap myFrameInfoImage;
        private UInt32[] myFrameInfo = new UInt32[0];
        private int myFrameNo;
        private zt.WpfLog log;

        private WriteableBitmap transparentBitmap() { return new WriteableBitmap(1, 1, 96, 96, PixelFormats.Bgra32, null); }
        private void writeFrameInfo()
        {
            if (myVideo != null)
            {
                if (myFrameInfoImage == null || myFrameInfoImage.PixelHeight != myFrameInfo.Length)
                {
                    myFrameInfoImage = new WriteableBitmap(1, myVideo.numFrames, 96, 96, PixelFormats.Bgr24, null);
                    NotifyPropertyChanged("FrameInfoImage");
                }
                myFrameInfoImage.WritePixels(new System.Windows.Int32Rect(0, 0, 1, myFrameInfo.Length), myFrameInfo, 4, 0, 0);
            }
        }
        private void makeFrameInfo()
        {
            if (myFrameInfo != null)
            {
                if (myKDTreeSource == null || myTrace == null)
                {
                    for (int i = 0; i < myFrameInfo.Length; i++) myFrameInfo[i] = 0xff7777;
                }
                else
                {
                    for (int i = 0; i < myFrameInfo.Length; i++)
                    {
                        myFrameInfo[i]
                            = (i == CurrentFrameNumber) ? 0xFF0000u
                            : !myKDTreeSource.IsReady(i) ? 0x7777FFu
                            : myTrace.IsFixed(i) ? 0x00AA00u
                            : myTrace.IsForceOccluded(i) ? 0x000000u
                            : 0xCCCCCCu;
                    }
                }
                writeFrameInfo();
            }
        }

        public ZooTracerModel()
        {
            log = new zt.WpfLog(WriteLine);
            StdOut = ImmutableList.Create("ZooTracer v.1");
            myFrameInfoImage = transparentBitmap();
            myWriteLineDelegate = new Action<string>(WriteLine);
            myNotifyPropertyChangedDelegate = new Action<string>(NotifyPropertyChanged);
        }

        public void LoadSettings()
        {
            Settings.Default.PropertyChanged += settingsChangedHandler;
            settingsChangedHandler(Settings.Default, new System.ComponentModel.PropertyChangedEventArgs("videofilepath"));
        }

        private void settingsChangedHandler(object s, System.ComponentModel.PropertyChangedEventArgs a)
        {
            if (a.PropertyName == "videofilepath")
            {
                var path = Settings.Default.videofilepath;
                if (string.IsNullOrWhiteSpace(path))
                {
                    startVideo(null);
                }
                else
                {
                    if (!System.IO.File.Exists(path)) WriteLine("File doesn' exist: " + path);
                    else
                    {
                        WriteLine("Trying to open video file to trace:");
                        WriteLine(path);
                        var video = new ztWpf.FrameSource(path);
                        if (video.numFrames <= 0) WriteLine("Cannot open the video.");
                        else
                            startVideo(Tuple.Create(path, video));
                    }
                }
            }
            else if (
                a.PropertyName == "patch_size"
                || a.PropertyName == "pca_dim"
                || a.PropertyName == "num_pca_data"
                )
            {
                myPcaTask = startPcaAsync();
            }
            else if (a.PropertyName == "index_accuracy")
            {
                var t = myPcaTask;
                if (t != null)
                    t.ContinueWith(tt => myKDTreeSource = startKDTreeSource(tt.Result), TaskContinuationOptions.NotOnCanceled | TaskContinuationOptions.NotOnFaulted);
            }
            else
                rerunTrace(myTrace);
        }

        private string myTraceFilePath = null;
        public string TraceFileName { get { return myTraceFilePath == null ? "(not set)" : Path.GetFileName(myTraceFilePath); } }
        public string TraceFileLocation { get { return Path.GetDirectoryName(myTraceFilePath); } }

        const string traceFileHeader = "x,y,code";
        const string traceMetaHeader = "Variable,Key,Type,Value";
        const string traceMetaStart = "0,videofilepath,System.String,";
        string[] read_csv_line(StreamReader r)
        {
            var line = r.ReadLine();
            if (line == null) return null;
            var parts = line.Split(',');
            if (parts.All(p => string.IsNullOrWhiteSpace(p))) return new string[0];
            return parts;
        }
        bool null_or_empty(Array a) { return a == null || a.Length == 0; }
        public void LoadTrace(string fileName)
        {
            myTraceFilePath = fileName;
            NotifyPropertyChanged("TraceFileName");
            NotifyPropertyChanged("TraceFileLocation");
            try
            {
                using (var tf = File.OpenText(fileName))
                {
                    var line = tf.ReadLine();
                    if (line.StartsWith(traceFileHeader))
                    {
                        var kps = new List<Tuple<double, double, int>>();
                        var ofs = new List<int>();
                        int f = 0;
                        string[] parts;
                        while (!null_or_empty(parts = read_csv_line(tf)))
                        {
                            if (parts.Length < 3) break;
                            var code = int.Parse(parts[2]);
                            if (code == 1) kps.Add(Tuple.Create(double.Parse(parts[0]), double.Parse(parts[1]), f));
                            else if (code == 2) ofs.Add(f);
                            f += 1;
                        }
                        if (parts != null && parts.Length == 0) // otherwise format error
                        {
                            while (!null_or_empty(parts = read_csv_line(tf))) ;
                            if (parts != null)
                            {
                                line = tf.ReadLine();
                                if (line.StartsWith(traceMetaHeader))
                                {
                                    line = tf.ReadLine();
                                    if (line.StartsWith(traceMetaStart))
                                    {
                                        var vfn = line.Substring(traceMetaStart.Length);
                                        while (vfn.Length > 0 && vfn[vfn.Length - 1] == ',') vfn = vfn.Substring(0, vfn.Length - 1);
                                        if (!File.Exists(vfn))
                                        {
                                            // try local path
                                            vfn = Path.Combine(Path.GetDirectoryName(fileName), Path.GetFileName(vfn));
                                        }
                                        if (!File.Exists(vfn))
                                        {
                                            WriteLine("Couldn't find video file. Tried the following paths:");
                                            WriteLine(line.Substring(traceMetaStart.Length));
                                            WriteLine(vfn);
                                            return;
                                        }
                                        var v = new ztWpf.FrameSource(vfn);
                                        if (v.numFrames <= 0)
                                        {
                                            WriteLine("Couldn't read the video file: " + vfn);
                                            return;
                                        }
                                        Settings.Default.PropertyChanged -= settingsChangedHandler;
                                        while (!null_or_empty(parts = read_csv_line(tf)))
                                        {
                                            if (parts.Length >= 4)
                                            {
                                                if (parts[1] == "lambda_d") Settings.Default.lambda_d = double.Parse(parts[3]);
                                                else if (parts[1] == "lambda_u") Settings.Default.lambda_u = double.Parse(parts[3]);
                                                else if (parts[1] == "lambda_o") Settings.Default.lambda_o = double.Parse(parts[3]);
                                                else if (parts[1] == "patch_size") Settings.Default.patch_size = int.Parse(parts[3]);
                                                else if (parts[1] == "num_pca_data") Settings.Default.num_pca_data = int.Parse(parts[3]);
                                                else if (parts[1] == "pca_dim") Settings.Default.pca_dim = int.Parse(parts[3]);
                                                else if (parts[1] == "max_occlusion_duration") Settings.Default.max_occlusion_duration = int.Parse(parts[3]);
                                                else if (parts[1] == "matches_per_keyframe") Settings.Default.matches_per_keyframe = int.Parse(parts[3]);
                                                else if (parts[1] == "index_approx_ratio") Settings.Default.index_approx_ratio = double.Parse(parts[3]);
                                                else if (parts[1] == "max_matches_per_frame") Settings.Default.max_matches_per_frame = int.Parse(parts[3]);
                                                else if (parts[1] == "matches_appearance_threshold") Settings.Default.matches_appearance_threshold = double.Parse(parts[3]);
                                                else if (parts[1] == "index_accuracy") Settings.Default.index_accuracy = int.Parse(parts[3]);
                                            }
                                        }
                                        Settings.Default.PropertyChanged += settingsChangedHandler;
                                        if (Settings.Default.videofilepath != vfn) Settings.Default.videofilepath = vfn;
                                        else myPcaTask = startPcaAsync();
                                        myPcaTask.ContinueWith(t =>
                                        {
                                            if (t.Status == TaskStatus.RanToCompletion)
                                            {
                                                var pca = t.Result;
                                                var trace = myTrace;
                                                var patch_size = Settings.Default.patch_size;
                                                var models = KeyPoints;
                                                if (pca != null && trace != null)
                                                {
                                                    foreach (var kp in kps)
                                                    {
                                                        var x = (int)Math.Round(kp.Item1 - 0.5 * patch_size);
                                                        var y = (int)Math.Round(kp.Item2 - 0.5 * patch_size);
                                                        var image = myVideo.getPatch(kp.Item3, new Int32Rect(x, y, patch_size, patch_size));
                                                        var descriptor = pca.getFeatures(kp.Item3, x, y, patch_size, v);
                                                        doSetKeyPoint(x, y, kp.Item3, image, descriptor, trace, models);
                                                    }
                                                    foreach (var of in ofs)
                                                    {
                                                        trace.Occlude(of);
                                                    }
                                                }
                                                else
                                                {
                                                    WriteLine("Error setting the trace points.");
                                                }
                                            }
                                        }, TaskScheduler.FromCurrentSynchronizationContext());
                                        return;
                                    }
                                }
                            }
                        }
                    }
                    WriteLine("The file has invalid format: " + fileName);
                }
            }
            catch (Exception e)
            {
                WriteLine("Error loading the trace: " + e.Message);
            }
        }
        public void SaveTrace(string fileName)
        {
            myTraceFilePath = fileName;
            NotifyPropertyChanged("TraceFileName");
            NotifyPropertyChanged("TraceFileLocation");
            try
            {
                var trace = myTrace;
                if (trace != null)
                    using (var f = File.CreateText(fileName))
                    {
                        f.WriteLine(traceFileHeader);
                        var patch_size = Settings.Default.patch_size;
                        for (int i = 0; i <= MaxFrameNumber; i++)
                        {
                            var loc = trace.GetTracePoint(i);
                            if (loc == null) f.WriteLine(",,{0}", trace.IsForceOccluded(i) ? 2 : 0);
                            else f.WriteLine("{1},{2},{0}", trace.IsFixed(i) ? 1 : 0, loc.x + 0.5 * patch_size, loc.y + 0.5 * patch_size);
                        }
                        f.WriteLine(@"
ID,Column,Variable Name,Data Type,Rank,Missing Value,Dimensions
1,A,x,System.Double,1,,frame
2,B,y,System.Double,1,,frame
3,C,code,System.Int16,1,,frame

{13}
{14}{0}
0,lambda_d,System.Double,{1}
0,lambda_u,System.Double,{2}
0,lambda_o,System.Double,{3}
0,patch_size,System.Double,{4}
0,num_pca_data,System.Double,{5}
0,pca_dim,System.Double,{6}
0,max_occlusion_duration,System.Double,{7}
0,matches_per_keyframe,System.Double,{8}
0,index_approx_ratio,System.Double,{9}
0,max_matches_per_frame,System.Double,{10}
0,matches_appearance_threshold,System.Double,{11}
0,index_accuracy,System.Double,{12}
",
                        Settings.Default.videofilepath,
                        Settings.Default.lambda_d,
                        Settings.Default.lambda_u,
                        Settings.Default.lambda_o,
                        Settings.Default.patch_size,
                        Settings.Default.num_pca_data,
                        Settings.Default.pca_dim,
                        Settings.Default.max_occlusion_duration,
                        Settings.Default.matches_per_keyframe,
                        Settings.Default.index_approx_ratio,
                        Settings.Default.max_matches_per_frame,
                        Settings.Default.matches_appearance_threshold,
                        Settings.Default.index_accuracy,
                        traceMetaHeader, traceMetaStart);
                    }
                WriteLine("Wrote the trace to " + fileName);
            }
            catch (Exception e)
            {
                WriteLine("Error saving the trace: " + e.Message);
            }
        }

        /// <summary>
        /// Specifies if a session is currently open.
        /// </summary>
        public bool IsOpen
        {
            get
            {
                return !string.IsNullOrWhiteSpace(myVideoPath);
            }
        }


        private Action<string> myNotifyPropertyChangedDelegate;
        protected void NotifyPropertyChanged(string propertyName)
        {
            Debug.Assert(!string.IsNullOrWhiteSpace(propertyName));
            if (System.Threading.Thread.CurrentThread != App.Current.Dispatcher.Thread)
                App.Current.Dispatcher.BeginInvoke(myNotifyPropertyChangedDelegate, propertyName);
            else
            {
                var handler = PropertyChanged;
                if (handler != null)
                    handler(this, new System.ComponentModel.PropertyChangedEventArgs(propertyName));
            }
        }

        #region video
        private string myVideoPath;
        /// <summary>
        /// The name of current video file or (null) if not set
        /// </summary>
        public string VideoFileName
        {
            get
            {
                return IsOpen ? System.IO.Path.GetFileName(myVideoPath) : "(not set)";
            }
        }

        /// <summary>
        /// The directory path of current video file or (null) if not set
        /// </summary>
        public string VideoFileLocation
        {
            get
            {
                return IsOpen ? System.IO.Path.GetDirectoryName(myVideoPath) : "";
            }
        }
        /// <summary>
        /// Text representation of video properties.
        /// </summary>
        public string VideoProperties
        {
            get
            {
                return IsOpen ? string.Format("{0}x{1}, {2}, {3}, {4}", myVideo.frameWidth, myVideo.frameHeight, myVideo.numFrames, myVideo.framesPerSecond, myVideo.fourCC) : "";
            }
        }
        private ztWpf.FrameSource myVideo;
        /// <summary>
        /// Closes current session (if any) and opens a new session with the specified video file.
        /// </summary>
        /// <param name="fileName">Video file name</param>
        public void startVideo(Tuple<string, ztWpf.FrameSource> args)
        {
            string fileName = args == null ? null : args.Item1;
            ztWpf.FrameSource v = args == null ? null : args.Item2;
            stopVideo(myVideo);
            myVideo = v;
            myVideoPath = fileName;
            var patch_size = Settings.Default.patch_size;
            myCursorImage = new WriteableBitmap(patch_size, patch_size, 96.0, 96.0, PixelFormats.Bgr24, null);
            NotifyPropertyChanged("VideoFileName");
            NotifyPropertyChanged("VideoFileLocation");
            NotifyPropertyChanged("VideoProperties");
            NotifyPropertyChanged("IsOpen");
            NotifyPropertyChanged("NumberOfFrames");
            NotifyPropertyChanged("MaxFrameNumber");
            myFrameImageSource = v == null ? null : v.getBitmap();
            NotifyPropertyChanged("FrameImageSource");
            myFrameInfo = new UInt32[v == null ? 0 : v.numFrames];
            myFrameNo = -1;
            CurrentFrameNumber = 0;
            WriteLine(VideoProperties);
            WriteLine();
            makeTransform();
            // open projector
            myPcaTask = v == null ? null : startPcaAsync();
        }

        private async Task stopVideo(ztWpf.FrameSource video)
        {
            if (video != null)
            {
                await stopPcaAsync();
                video.Dispose();
            }
        }
        #endregion
        #region trace
        private ztWpf.Trace startTrace(ztWpf.KDTreeSource index)
        {
            var trace = new ztWpf.Trace(index,
                Settings.Default.index_approx_ratio,
                Settings.Default.matches_per_keyframe,
                Settings.Default.max_matches_per_frame,
                Settings.Default.matches_appearance_threshold,
                Settings.Default.lambda_d,
                Settings.Default.lambda_u,
                Settings.Default.lambda_o,
                Settings.Default.max_occlusion_duration);
            trace.Subscribe(new ztWpf.Trace.ProgressHandler((i, j) =>
            {
                var app = Application.Current;
                if (app != null) app.Dispatcher.BeginInvoke(new Action(MakeGlassItems));
            }));
            return trace;
        }

        private static void rerunTrace(ztWpf.Trace trace)
        {
            if (trace != null)
                trace.Rerun(Settings.Default.index_approx_ratio,
                Settings.Default.matches_per_keyframe,
                Settings.Default.max_matches_per_frame,
                Settings.Default.matches_appearance_threshold,
                Settings.Default.lambda_d,
                Settings.Default.lambda_u,
                Settings.Default.lambda_o,
                Settings.Default.max_occlusion_duration);
        }

        private async Task stopTraceAsync(ztWpf.Trace trace)
        {
            if (trace != null)
                await Task.Run(new Action(trace.Dispose));
        }
        #endregion
        #region KDTreeSource
        private ztWpf.Trace myTrace;
        private ztWpf.KDTreeSource startKDTreeSource(ztWpf.Projector pca)
        {
            if (pca == null) return null;
            int workers = 2;
            int.TryParse(Environment.GetEnvironmentVariable("NUMBER_OF_PROCESSORS"), out workers);
            workers = Math.Max(1, workers - 1);
            var index = new ztWpf.KDTreeSource(myVideo, pca, Settings.Default.index_accuracy, workers, getPcaFolderPath());
            myTrace = startTrace(index);
            index.Subscribe(progressHandlerKDTreeSource);
            return index;
        }
        private bool frameInfoUpdateInProgress = false;
        private void progressHandlerKDTreeSource(int framesComplete)
        {
            rerunTrace(myTrace);
            if (framesComplete >= MaxFrameNumber || !frameInfoUpdateInProgress)
            {
                frameInfoUpdateInProgress = true;
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    makeFrameInfo();
                    frameInfoUpdateInProgress = false;
                }));
            }
        }
        private async Task stopKDTreeSource(ztWpf.KDTreeSource index)
        {
            var trc = myTrace;
            myTrace = null;
            await stopTraceAsync(trc);
            if (index != null)
                await Task.Run(new Action(index.Dispose));
        }
        #endregion
        #region pca
        private Task<ztWpf.Projector> myPcaTask = null;
        private CancellationTokenSource myPcaCTS;
        private ztWpf.KDTreeSource myKDTreeSource = null;
        private static string getPcaFolderPath()
        {
            var path = Settings.Default.videofilepath;
            if (!File.Exists(path)) throw new FileNotFoundException("video file", path);
            var projPath = path + ".zootracer";
            if (!Directory.Exists(projPath)) Directory.CreateDirectory(projPath);
            var pcaPath = Path.Combine(projPath, string.Format("{0}_{1}_{2}", Settings.Default.patch_size, Settings.Default.pca_dim, Settings.Default.num_pca_data));
            if (!Directory.Exists(pcaPath)) Directory.CreateDirectory(pcaPath);
            return pcaPath;
        }
        private static string getProjectorFilePath()
        {
            return Path.Combine(getPcaFolderPath(), "pca");
        }
        private ImageSource[] myPrincipals = null;
        /// <summary>
        /// Images of projector principal compomnents.
        /// </summary>
        public ImageSource[] ProjectorPrincipals { get { return myPrincipals; } }

        private async Task<ztWpf.Projector> startPcaAsync()
        {
            try
            {
                await stopPcaAsync();
                myPcaCTS = new CancellationTokenSource();
                var fn = getProjectorFilePath();
                if (!File.Exists(fn))
                {
                    await startBuildingProjector(myPcaCTS.Token);
                }
                WriteLine("Reading pca file:");
                WriteLine(fn);
                var pca = new ztWpf.Projector(fn);
                myPrincipals = pca.getPrincipals();
                NotifyPropertyChanged("ProjectorPrincipals");
                myKDTreeSource = startKDTreeSource(pca);
                return pca;
            }
            catch (OperationCanceledException)
            {
                WriteLine("Cancelled pca start.");
                return null;
            }
            catch (Exception e)
            {
                WriteLine("Error building pca:");
                WriteLine(e.Message);
                return null;
            }
        }

        /// <summary>
        /// Starts a task that represents building PCA in a separate process.
        /// </summary>
        /// <param name="token">Cancelling the token kills the PCA build process and moves the returned task to the Cancelled state.</param>
        /// <returns>A task representing the process. Task result is true if the process exit code equals zero.</returns>
        private Task startBuildingProjector(CancellationToken token)
        {
            Properties.Settings.Default.Save();
            var tcs = new TaskCompletionSource<bool>();
            var p = new Process();
            ExitEventHandler h1 = (s, a) => { try { p.Kill(); } catch { } };
            Application.Current.Exit += h1;
            token.Register(() => { tcs.TrySetCanceled(); p.Kill(); });
            p.StartInfo.FileName = System.Reflection.Assembly.GetExecutingAssembly().Location;
            p.StartInfo.Arguments = "build_pca";
            p.StartInfo.UseShellExecute = false;
            p.StartInfo.RedirectStandardOutput = true;
            p.StartInfo.RedirectStandardError = true;
            p.EnableRaisingEvents = true;
            DataReceivedEventHandler h0 = (s, a) => { WriteLine(a.Data); };
            p.OutputDataReceived += h0;
            p.ErrorDataReceived += h0;
            p.Exited += (s, a) =>
            {
                var app = Application.Current;
                if (app != null)
                    app.Dispatcher.BeginInvoke(new Action<Application>(d => { d.Exit -= h1; }), app);
                var sender = s as Process;
                tcs.TrySetResult(sender != null && sender.HasExited && sender.ExitCode == 0);
            };
            p.Start();
            p.BeginOutputReadLine();
            p.BeginErrorReadLine();
            return tcs.Task;
        }
        /// <summary>
        /// Builds the PCA projector file reporting progress to stdout.
        /// </summary>
        public static ztWpf.Projector BuildProjector()
        {
            var path = Settings.Default.videofilepath;
            if (!System.IO.File.Exists(path)) return null;
            var v = new ztWpf.FrameSource(path);
            int patch_size = Settings.Default.patch_size;
            int output_dim = Settings.Default.pca_dim;
            int number_of_samples = Settings.Default.num_pca_data;
            Console.WriteLine(string.Format("Generating new projector from {2} patches {1}x{1} px size. Output dimension = {0}.", output_dim, patch_size, number_of_samples));
            return new ztWpf.Projector(v, output_dim, patch_size, number_of_samples, getProjectorFilePath(), Console.WriteLine);
        }

        private async Task stopPcaAsync()
        {
            myPrincipals = null;
            NotifyPropertyChanged("ProjectorPrincipals");
            var cts = myPcaCTS;
            var tsk = myPcaTask;
            myPcaTask = null;
            var idx = myKDTreeSource;
            myKDTreeSource = null;
            makeFrameInfo();
            await stopKDTreeSource(idx);
            if (tsk != null)
            {
                cts.Cancel();
                var pca = await tsk;
                if (pca != null)
                    pca.Dispose();
            }
        }
        #endregion

        public event System.ComponentModel.PropertyChangedEventHandler PropertyChanged;

        /// <summary>
        /// Returns a writable bitmap that can be linked to a WPF Image control.
        /// </summary>
        public ImageSource FrameImageSource
        {
            get
            {
                if (IsOpen) return myFrameImageSource;
                return Application.Current.Resources["NoImageSource"] as ImageSource;
            }
        }

        /// <summary>
        /// Gets or sets current video frame number.
        /// </summary>
        public int CurrentFrameNumber
        {
            get
            {
                return IsOpen ? myFrameNo : 0;
            }
            set
            {
                if (IsOpen && value != myFrameNo)
                {
                    myFrameNo = value;
                    NotifyPropertyChanged("CurrentFrameNumber");
                    myVideo.writeFrame(myFrameImageSource, value);
                    MakeGlassItems();
                    makeMatchImages();
                    makeFrameInfo();
                }
            }
        }

        public int MaxFrameNumber
        {
            get
            {
                return IsOpen ? myVideo.numFrames - 1 : 0;
            }
        }



        /// <summary>
        /// Contents of console output.
        /// </summary>
        public ImmutableList<string> StdOut { get; private set; }


        private Action<string> myWriteLineDelegate;
        /// <summary>
        /// Adds a line to console output.
        /// </summary>
        /// <param name="line">The line to add to console output.</param>
        public void WriteLine(string line = "")
        {
            if (line == null) return;
            var app = App.Current;
            if (app == null) return;
            if (System.Threading.Thread.CurrentThread != app.Dispatcher.Thread)
                App.Current.Dispatcher.BeginInvoke(myWriteLineDelegate, line);
            else
            {
                StdOut = StdOut.Add(line);
                NotifyPropertyChanged("StdOut");
            }
        }

        static private bool isCursor(GlassItem gi) { return gi.Tag == GlassItem.GlassItemTag.Cursor; }
        static private bool isMatch(GlassItem gi) { return gi.Tag == GlassItem.GlassItemTag.Similar; }
        private void glassClearCursor()
        {
            var i = GlassItems.FirstOrDefault(isCursor);
            while (i != null)
            {
                GlassItems.Remove(i);
                i = GlassItems.FirstOrDefault(isCursor);
            }
        }

        private void glassAddMatches(List<Tuple<int, int, double>> matches)
        {
            var i = GlassItems.FirstOrDefault(isMatch);
            while (i != null)
            {
                GlassItems.Remove(i);
                i = GlassItems.FirstOrDefault(isMatch);
            }
            if (matches != null) foreach (var m in matches)
                {
                    var patch_size = Settings.Default.patch_size;
                    GlassItems.Insert(0, new GlassRectModel(
                                m.Item1, m.Item2, patch_size, patch_size, glassScale, glassOffsetX, glassOffsetY,
                                Brushes.Blue, 2, GlassItem.GlassItemTag.Similar));
                }
        }

        private Tuple<int, int> myCursorPosition = null;
        private Tuple<int, int> myPatchPosition = null;
        private float[] myPatchDescriptor = new float[0];
        /// <summary>
        /// Position of a user controlled patch.
        /// </summary>
        public Tuple<int, int> CursorPosition
        {
            get
            {
                return myCursorPosition;
            }
            set
            {
                if (value != myCursorPosition)
                {
                    myCursorPosition = value;
                    NotifyPropertyChanged("CursorPosition");
                    if (value == null)
                    {
                        glassClearCursor();
                        NotifyPropertyChanged("CursorImage");
                        glassAddMatches(null);
                    }
                    else if (myVideo != null)
                    {
                        var patch_size = Settings.Default.patch_size;
                        myPatchPosition = myVideo.writeFrame(myCursorImage, CurrentFrameNumber, value.Item1 - patch_size / 2, value.Item2 - patch_size / 2);
                        glassClearCursor();
                        myGlassItems.Add(new GlassRectModel(
                            myPatchPosition.Item1, myPatchPosition.Item2, patch_size, patch_size, glassScale, glassOffsetX, glassOffsetY,
                            Brushes.Yellow, 2, GlassItem.GlassItemTag.Cursor));
                        NotifyPropertyChanged("CursorImage");
                        var t = myPcaTask;
                        var pca = t != null && t.IsCompleted ? t.Result : null;
                        if (pca != null)
                        {
                            myPatchDescriptor = pca.getFeatures(CurrentFrameNumber, myPatchPosition.Item1, myPatchPosition.Item2, patch_size, myVideo);
                            if (myKDTreeSource != null && PageIndex == matchingPageIndex)
                            {
                                var matches = myKDTreeSource.getMatches(CurrentFrameNumber, myPatchDescriptor, 5, pca);
                                glassAddMatches(matches);
                            }
                        }
                    }
                }
            }
        }


        WriteableBitmap myCursorImage;
        /// <summary>
        /// A patch image under cursor.
        /// </summary>
        public ImageSource CursorImage
        {
            get
            {
                if (myCursorPosition == null) return null;
                else return myCursorImage;
            }
        }

        double glassScale = 1.0;
        double glassOffsetX = 0;
        double glassOffsetY = 0;
        void makeTransform()
        {
            var items = ImmutableList.CreateRange(myGlassItems);
            myGlassItems.Clear();
            if (myVideo == null || myVideo.numFrames <= 0 || GlassWidth <= 0 || GlassHeight <= 0)
            {
                glassScale = 1.0;
                glassOffsetX = 0;
                glassOffsetY = 0;
            }
            else
            {
                double fw = myVideo.frameWidth;
                double fh = myVideo.frameHeight;
                var iw = GlassWidth;
                var ih = GlassHeight;
                // scale the position to frame pixel coords
                glassScale = Math.Min(iw / fw, ih / fh); // image = scale*frame
                glassOffsetX = 0.5 * (iw - fw * glassScale);
                glassOffsetY = 0.5 * (ih - fh * glassScale);
            }
            foreach (var that in items)
                myGlassItems.Add(that.Rescale(glassScale, glassOffsetX, glassOffsetY));
        }
        private double myGlassWidth = 0.0;
        public double GlassWidth
        {
            private get { return myGlassWidth; }
            set
            {
                myGlassWidth = value;
                makeTransform();
            }
        }
        private double myGlassHeight = 0.0;
        public double GlassHeight
        {
            private get { return myGlassHeight; }
            set
            {
                myGlassHeight = value;
                makeTransform();
            }
        }
        ObservableCollection<GlassItem> myGlassItems = new ObservableCollection<GlassItem>();
        public ObservableCollection<GlassItem> GlassItems { get { return myGlassItems; } }

        #region user commands
        /// <summary>
        /// Force trace position at current frame.
        /// </summary>
        public void SetKeyPoint()
        {
            var trace = myTrace;
            if (trace != null)
            {
                var patch_size = Settings.Default.patch_size;
                var image = myVideo.getPatch(CurrentFrameNumber, new Int32Rect(myPatchPosition.Item1, myPatchPosition.Item2, patch_size, patch_size));
                doSetKeyPoint(myPatchPosition.Item1, myPatchPosition.Item2, CurrentFrameNumber, image, myPatchDescriptor, trace, KeyPoints);
                MakeGlassItems();
            }
        }
        private static void doSetKeyPoint(int x, int y, int frame, ImageSource image, float[] descriptor, ztWpf.Trace trace, IList<KeyPointModel> kps)
        {
            trace.Fix(frame, new ztWpf.Patch(x, y), descriptor);

            // find index to insert
            int i = 0; while (i < kps.Count && kps[i].FrameNumber < frame) ++i;
            var kp = new KeyPointModel()
            {
                FrameNumber = frame,
                Image = image,
                Descriptor = descriptor
            };
            if (i >= kps.Count)
                kps.Add(kp);
            else if (kps[i].FrameNumber == frame)
                kps[i] = kp;
            else
                kps.Insert(i, kp);
        }

        /// <summary>
        /// Force trace occluded at current frame.
        /// </summary>
        public void SetOccludedFrame()
        {
            if (myTrace != null)
            {
                myTrace.Occlude(CurrentFrameNumber);
                var kps = KeyPoints;
                // find index to insert
                int i = 0; while (i < kps.Count && kps[i].FrameNumber < CurrentFrameNumber) ++i;
                if (i < kps.Count && kps[i].FrameNumber == CurrentFrameNumber) kps.RemoveAt(i);
            }
        }

        /// <summary>
        /// Clear user trace info at current frame and allow the tool to automatically select trace position.
        /// </summary>
        public void SetAutoFrame()
        {
            if (myTrace != null)
            {
                myTrace.Clear(CurrentFrameNumber);
                var kps = KeyPoints;
                // find index to insert
                int i = 0; while (i < kps.Count && kps[i].FrameNumber < CurrentFrameNumber) ++i;
                if (i < kps.Count && kps[i].FrameNumber == CurrentFrameNumber) kps.RemoveAt(i);
            }
        }
        /// <summary>
        /// Reset the trace, clearing all user supplied info
        /// </summary>
        public void ClearAll()
        {
            var index = myKDTreeSource;
            if (index != null)
            {
                stopTraceAsync(myTrace);
                myTrace = startTrace(index);
                myKeyPoints.Clear();
                MakeGlassItems();
                makeFrameInfo();
            }
        }
        #endregion
        /// <summary>
        /// A bitmap that represents state of each frame.
        /// </summary>
        public ImageSource FrameInfoImage
        {
            get
            {
                return myFrameInfoImage;
            }
        }

        public string FrameTraceState
        {
            get
            {
                var trace = myTrace;
                if (trace == null) return null;
                else
                {
                    var loc = trace.GetTracePoint(CurrentFrameNumber);
                    if (loc == null)
                    {
                        if (trace.IsForceOccluded(CurrentFrameNumber)) return "occluded";
                        else return "(occluded)";
                    }
                    else
                    {
                        var s = string.Format("{0},{1}", loc.x, loc.y);
                        if (trace.IsFixed(CurrentFrameNumber)) return s;
                        else return "(" + s + ")";
                    }
                }
            }
        }

        /// <summary>
        /// The selected tab index.
        /// </summary>
        public int PageIndex { get; set; }
        const int matchingPageIndex = 4;

        public void MakeGlassItems()
        {
            var cursor = GlassItems.FirstOrDefault(isCursor);
            GlassItems.Clear();
            if (PageIndex == 1 && myTrace != null)
            {
                // path
                var startFrame = Math.Max(0, CurrentFrameNumber - 100);
                var endFrame = Math.Min(MaxFrameNumber, CurrentFrameNumber + 100);
                var p = myTrace.GetTracePoint(startFrame);
                var patch_size = Settings.Default.patch_size;
                while (p == null && startFrame < endFrame) { p = myTrace.GetTracePoint(++startFrame); }
                if (p != null)
                {
                    var startPoint = new Point(p.x + 0.5 * patch_size, p.y + 0.5 * patch_size);
                    var segments = new List<PathSegment>();
                    while (startFrame < endFrame)
                    {
                        p = myTrace.GetTracePoint(++startFrame);
                        if (p == null)
                        {
                            while (p == null && startFrame < endFrame) { p = myTrace.GetTracePoint(++startFrame); }
                            if (p != null) segments.Add(new LineSegment(new Point(p.x + 0.5 * patch_size, p.y + 0.5 * patch_size), false));
                        }
                        else
                            segments.Add(new LineSegment(new Point(p.x + 0.5 * patch_size, p.y + 0.5 * patch_size), true));
                    }
                    if (segments.Count > 0)
                        GlassItems.Add(new GlassPathModel(
                            new PathGeometry(new PathFigure[] { new PathFigure(startPoint, segments, false) }),
                            glassScale, glassOffsetX, glassOffsetY,
                            Brushes.Blue, 2, GlassRectModel.GlassItemTag.Trace));
                }
                // current trace point
                p = myTrace.GetTracePoint(CurrentFrameNumber);
                if (p != null)
                {
                    if (myTrace.IsFixed(CurrentFrameNumber))
                    {
                        GlassItems.Add(new GlassRectModel(
                                    p.x, p.y, patch_size, patch_size, glassScale, glassOffsetX, glassOffsetY,
                                    Brushes.LightBlue, 4, GlassRectModel.GlassItemTag.Trace));
                    }
                    else
                    {
                        GlassItems.Insert(0, new GlassRectModel(
                                    p.x, p.y, patch_size, patch_size, glassScale, glassOffsetX, glassOffsetY,
                                    Brushes.Blue, 2, GlassRectModel.GlassItemTag.Trace));
                    }
                }
            }
            else if (PageIndex == matchingPageIndex)
            {
                for (int i = 0; i < myMatchImages.Count; i++)
                {
                    var loc = myMatchImages[i].Location;
                    GlassItems.Insert(0, new GlassRectModel(
                                    loc.X, loc.Y, loc.Width, loc.Height, glassScale, glassOffsetX, glassOffsetY,
                                    i == MatchSelectedImage ? Brushes.Red : Brushes.Blue, 2, GlassRectModel.GlassItemTag.Trace));
                }
            }
            if (cursor != null) GlassItems.Add(cursor);
            NotifyPropertyChanged("FrameTraceState");
            makeFrameInfo();
        }

        private ObservableCollection<KeyPointModel> myKeyPoints = new ObservableCollection<KeyPointModel>();
        public ObservableCollection<KeyPointModel> KeyPoints
        {
            get
            {
                return myKeyPoints;
            }
        }

        /// <summary>
        /// Fill in myMatchImages when a key point or a frame is selected.
        /// </summary>
        private void makeMatchImages()
        {
            myMatchImages.Clear();
            if (KeyPointSelectedImage >= 0 && KeyPointSelectedImage < KeyPoints.Count && KeyPoints[KeyPointSelectedImage].Descriptor != null
                && myKDTreeSource != null && myKDTreeSource.IsReady(CurrentFrameNumber) && myPcaTask.IsCompleted)
            {
                var pca = myPcaTask.Result;
                if (pca != null)
                {
                    var matches = myKDTreeSource.getMatches(CurrentFrameNumber, KeyPoints[KeyPointSelectedImage].Descriptor, 10, pca);
                    var patch_size = Settings.Default.patch_size;
                    foreach (var match in matches)
                    {
                        var loc = new Int32Rect(match.Item1, match.Item2, patch_size, patch_size);
                        var img = myVideo.getPatch(CurrentFrameNumber, loc);
                        var descr = pca.getFeatures(CurrentFrameNumber, match.Item1, match.Item2, patch_size, myVideo);
                        myMatchImages.Add(new MatchPointModel()
                        {
                            Image = img,
                            Location = loc,
                            Distance = match.Item3,
                            Descriptor = descr
                        });
                    }
                }
            }
        }
        private ObservableCollection<MatchPointModel> myMatchImages = new ObservableCollection<MatchPointModel>();
        public ObservableCollection<MatchPointModel> MatchImages
        {
            get
            {
                return myMatchImages;
            }
        }

        private ImageSource myKeyPointReconstructedImage = null;
        public ImageSource KeyPointReconstructedImage
        {
            get
            {
                return myKeyPointReconstructedImage;
            }
        }

        private ImageSource myMatchReconstructedImage = null;
        public ImageSource MatchReconstructedImage
        {
            get
            {
                return myMatchReconstructedImage;
            }
        }

        private int myKeyPointSelectedImage = -1;
        public int KeyPointSelectedImage
        {
            get
            {
                return myKeyPointSelectedImage;
            }
            set
            {
                if (value != myKeyPointSelectedImage)
                {
                    myKeyPointSelectedImage = value;
                    NotifyPropertyChanged("KeyPointSelectedImage");
                    var t = myPcaTask;
                    var pca = t == null || !t.IsCompleted ? null : t.Result;
                    if (pca != null && value >= 0 && value < myKeyPoints.Count && myKeyPoints[value].Descriptor != null)
                        myKeyPointReconstructedImage = pca.reconstruct(myKeyPoints[value].Descriptor);
                    else
                        myKeyPointReconstructedImage = null;
                    NotifyPropertyChanged("KeyPointReconstructedImage");
                    makeMatchImages();
                    MakeGlassItems();
                }
            }
        }

        private int myMatchSelectedImage = -1;
        public int MatchSelectedImage
        {
            get
            {
                return myMatchSelectedImage;
            }
            set
            {
                if (value != myMatchSelectedImage)
                {
                    myMatchSelectedImage = value;
                    NotifyPropertyChanged("MatchSelectedImage");
                    var t = myPcaTask;
                    var pca = t == null || !t.IsCompleted ? null : t.Result;
                    if (pca != null && value >= 0 && value < myMatchImages.Count)
                        myMatchReconstructedImage = pca.reconstruct(myMatchImages[value].Descriptor);
                    else
                        myMatchReconstructedImage = null;
                    NotifyPropertyChanged("MatchReconstructedImage");
                    MakeGlassItems();
                }
            }
        }

        public bool CanTrace { get { return myTrace != null; } }

        public bool IsBeforeTraceEnd()
        {
            if (myTrace != null)
                for (int i = CurrentFrameNumber; i <= MaxFrameNumber; ++i)
                    if (!myTrace.IsForceOccluded(i)) return true;
            return false;
        }

        public bool IsAfterTraceStart()
        {
            if (myTrace != null)
                for (int i = CurrentFrameNumber; i >= 0; --i)
                    if (!myTrace.IsForceOccluded(i)) return true;
            return false;
        }

        public void StartHere()
        {
            if (myTrace != null)
            {
                if (IsAfterTraceStart())
                    for (int i = CurrentFrameNumber - 1; i >= 0; --i)
                        myTrace.Occlude(i);
                else
                    for (int i = CurrentFrameNumber; i <= MaxFrameNumber; ++i)
                        if (myTrace.IsForceOccluded(i)) myTrace.Clear(i);
            }
        }

        public void StopHere()
        {
            if (myTrace != null)
            {
                if (IsBeforeTraceEnd())
                    for (int i = CurrentFrameNumber + 1; i <= MaxFrameNumber; ++i)
                        myTrace.Occlude(i);
                else
                    for (int i = CurrentFrameNumber; i >= 0; --i)
                        if (myTrace.IsForceOccluded(i)) myTrace.Clear(i);
            }
        }

        public string Version
        {
            get
            {
                return this.GetType().Assembly.GetName().Version.ToString();
            }
        }
    }

}
