using System.Windows.Media;

namespace ZooTracer
{
    public abstract class GlassItem
    {
        public enum GlassItemTag { Unknown, Cursor, Similar, Trace }
        public GlassItemTag Tag { get; set; }
        public GlassItem(GlassItemTag tag)
        {
            Tag = tag;
        }
        public abstract GlassItem Rescale(double scale, double offsetX, double offsetY);
    }
}
