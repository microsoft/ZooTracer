using System.Windows.Media;

namespace ZooTracer
{
    public class GlassPathModel:GlassItem
    {
        public Geometry PathData { get; private set; }
        public Brush Stroke { get; set; }
        public double Thickness { get; set; }
        public Transform Transform { get; private set; }
        public GlassPathModel(Geometry path, double scale, double offsetX, double offsetY, Brush stroke, double thickness, GlassItemTag tag)
            : base(tag)
        {
            PathData = path;  
            Stroke = stroke; Thickness = thickness; Tag = tag;
            var r = new TransformGroup();
            r.Children.Add(new ScaleTransform(scale, scale));
            r.Children.Add(new TranslateTransform(offsetX, offsetY));
            Transform = r;
        }
        public override GlassItem Rescale(double scale, double offsetX, double offsetY)
        {
            return new GlassPathModel(PathData, scale, offsetX, offsetY, Stroke, Thickness, Tag);
        }
    }
}
