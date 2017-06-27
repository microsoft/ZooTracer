using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media;

namespace ZooTracer
{
    public class KeyPointModel
    {
        public ImageSource Image { get; set; }
        public int FrameNumber { get; set; }
        public float[] Descriptor { get; set; }
    }
}
