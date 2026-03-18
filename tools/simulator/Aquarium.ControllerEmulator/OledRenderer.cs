using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace Aquarium.ControllerEmulator;

public sealed class OledRenderer : IDisposable
{
  public const int Width = 128;
  public const int Height = 32;

  private readonly int[] _argb = new int[Width * Height];
  private readonly Bitmap _bitmap = new(Width, Height, PixelFormat.Format32bppArgb);
  private bool _dirty = true;

  public void UpdateFromNative(ReadOnlySpan<byte> source)
  {
    if (source.Length < Width * Height)
    {
      return;
    }

    for (var y = 0; y < Height; y++)
    {
      for (var x = 0; x < Width; x++)
      {
        var nativeIndex = x * Height + y;
        var linearIndex = y * Width + x;
        _argb[linearIndex] = source[nativeIndex] == 0
          ? unchecked((int)0xFF000000)
          : unchecked((int)0xFFFFFFFF);
      }
    }

    _dirty = true;
  }

  public void Render(Graphics graphics, Rectangle target)
  {
    graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
    graphics.PixelOffsetMode = PixelOffsetMode.Half;
    graphics.SmoothingMode = SmoothingMode.None;
    graphics.CompositingQuality = CompositingQuality.HighSpeed;
    graphics.Clear(Color.Black);
    graphics.DrawImage(GetBitmap(), target);
  }

  private Bitmap GetBitmap()
  {
    if (!_dirty)
    {
      return _bitmap;
    }

    var rect = new Rectangle(0, 0, Width, Height);
    var data = _bitmap.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
    try
    {
      Marshal.Copy(_argb, 0, data.Scan0, _argb.Length);
    }
    finally
    {
      _bitmap.UnlockBits(data);
    }

    _dirty = false;
    return _bitmap;
  }

  public void Dispose()
  {
    _bitmap.Dispose();
  }
}

public sealed class OledDisplayControl : Control
{
  public OledRenderer? Renderer { get; set; }

  public OledDisplayControl()
  {
    DoubleBuffered = true;
    BackColor = Color.Black;
    MinimumSize = new Size(512, 128);
    MaximumSize = new Size(512, 128);
    Size = new Size(512, 128);
  }

  protected override void OnPaint(PaintEventArgs e)
  {
    base.OnPaint(e);

    e.Graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
    e.Graphics.PixelOffsetMode = PixelOffsetMode.Half;
    e.Graphics.SmoothingMode = SmoothingMode.None;
    e.Graphics.CompositingQuality = CompositingQuality.HighSpeed;

    if (Renderer is null)
    {
      e.Graphics.Clear(Color.Black);
      return;
    }

    var scaleX = Math.Max(1, ClientSize.Width / OledRenderer.Width);
    var scaleY = Math.Max(1, ClientSize.Height / OledRenderer.Height);
    var scale = Math.Max(1, Math.Min(scaleX, scaleY));
    var width = OledRenderer.Width * scale;
    var height = OledRenderer.Height * scale;
    var x = (ClientSize.Width - width) / 2;
    var y = (ClientSize.Height - height) / 2;

    Renderer.Render(e.Graphics, new Rectangle(x, y, width, height));
  }
}
