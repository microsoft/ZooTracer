using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace ZooTracer
{
    public class MatchPointModel
    {
        public ImageSource Image { get; set; }
        public Int32Rect Location { get; set; }
        public double Distance { get; set; }
        public float[] Descriptor { get; set; }
    }
}
