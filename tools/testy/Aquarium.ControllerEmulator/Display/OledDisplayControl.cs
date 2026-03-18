using System.Drawing.Drawing2D;

namespace Aquarium.ControllerEmulator.Display;

public sealed class OledDisplayControl : Control
{
  private OledRenderer? _renderer;

  public OledDisplayControl()
  {
    DoubleBuffered = true;
    BackColor = Color.Black;
    ForeColor = Color.White;
    MinimumSize = new Size(512, 128);
    MaximumSize = new Size(512, 128);
    Size = new Size(512, 128);
    ResizeRedraw = true;
  }

  public OledRenderer? Renderer
  {
    get => _renderer;
    set
    {
      _renderer = value;
      Invalidate();
    }
  }

  protected override void OnPaint(PaintEventArgs e)
  {
    base.OnPaint(e);

    e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
    e.Graphics.PixelOffsetMode = PixelOffsetMode.Half;
    e.Graphics.SmoothingMode = SmoothingMode.None;
    e.Graphics.CompositingQuality = CompositingQuality.HighSpeed;
    e.Graphics.Clear(Color.Black);

    if (_renderer is null)
    {
      return;
    }

    var scaleX = Math.Max(1, ClientSize.Width / OledRenderer.Width);
    var scaleY = Math.Max(1, ClientSize.Height / OledRenderer.Height);
    var scale = Math.Max(1, Math.Min(scaleX, scaleY));

    var width = OledRenderer.Width * scale;
    var height = OledRenderer.Height * scale;
    var x = (ClientSize.Width - width) / 2;
    var y = (ClientSize.Height - height) / 2;

    _renderer.Render(e.Graphics, new Rectangle(x, y, width, height));
  }
}
