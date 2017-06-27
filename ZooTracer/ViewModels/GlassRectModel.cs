using System.Windows.Media;

namespace ZooTracer
{
    public class GlassRectModel : GlassItem
    {
        public int X { get; private set; }
        public int Y { get; private set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public Brush Stroke { get; set; }
        public double Thickness { get; set; }
        public Transform Transform { get; private set; }
        public GlassRectModel(int x, int y, int w, int h, double scale, double offsetX, double offsetY, Brush stroke, double thickness, GlassItemTag tag)
            : base(tag)
        {
            X = x; Y = y; Width = w; Height = h; Stroke = stroke; Thickness = thickness;
            var r = new TransformGroup();
            r.Children.Add(new ScaleTransform(scale, scale));
            r.Children.Add(new TranslateTransform(x * scale + offsetX, y * scale + offsetY));
            Transform = r;
        }
        public override GlassItem Rescale(double scale, double offsetX, double offsetY)
        {
            return new GlassRectModel(X, Y, Width, Height, scale, offsetX, offsetY, Stroke, Thickness, Tag);
        }
    }
}
