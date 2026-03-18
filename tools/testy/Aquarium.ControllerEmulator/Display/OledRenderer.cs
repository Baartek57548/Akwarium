using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace Aquarium.ControllerEmulator.Display;

public sealed class OledRenderer : IDisposable
{
  private readonly byte[] _pixels = new byte[Width * Height];
  private readonly int[] _argb = new int[Width * Height];
  private readonly Bitmap _framebuffer = new(Width, Height, PixelFormat.Format32bppArgb);
  private readonly BitmapFont _font;
  private bool _isDirty = true;

  public OledRenderer(BitmapFont? font = null)
  {
    _font = font ?? new BitmapFont();
    Clear();
  }

  public const int Width = 128;
  public const int Height = 32;

  public void Clear(bool setPixels = false)
  {
    Array.Fill(_pixels, setPixels ? (byte)1 : (byte)0);
    _isDirty = true;
  }

  public void DrawPixel(int x, int y, bool isOn = true)
  {
    if (x < 0 || x >= Width || y < 0 || y >= Height)
    {
      return;
    }

    _pixels[(y * Width) + x] = isOn ? (byte)1 : (byte)0;
    _isDirty = true;
  }

  public void DrawRect(int x, int y, int width, int height, bool filled = false)
  {
    if (width <= 0 || height <= 0)
    {
      return;
    }

    if (filled)
    {
      for (var py = y; py < y + height; py++)
      {
        for (var px = x; px < x + width; px++)
        {
          DrawPixel(px, py);
        }
      }

      return;
    }

    for (var px = x; px < x + width; px++)
    {
      DrawPixel(px, y);
      DrawPixel(px, y + height - 1);
    }

    for (var py = y; py < y + height; py++)
    {
      DrawPixel(x, py);
      DrawPixel(x + width - 1, py);
    }
  }

  public void DrawLine(int x0, int y0, int x1, int y1)
  {
    var dx = Math.Abs(x1 - x0);
    var sx = x0 < x1 ? 1 : -1;
    var dy = -Math.Abs(y1 - y0);
    var sy = y0 < y1 ? 1 : -1;
    var error = dx + dy;

    while (true)
    {
      DrawPixel(x0, y0);
      if (x0 == x1 && y0 == y1)
      {
        break;
      }

      var twiceError = 2 * error;
      if (twiceError >= dy)
      {
        error += dy;
        x0 += sx;
      }

      if (twiceError <= dx)
      {
        error += dx;
        y0 += sy;
      }
    }
  }

  public void DrawBitmap(int x, int y, int width, int height, ReadOnlySpan<byte> packedBitmap)
  {
    if (width <= 0 || height <= 0)
    {
      return;
    }

    var bytesPerRow = (width + 7) / 8;
    var requiredLength = bytesPerRow * height;
    if (packedBitmap.Length < requiredLength)
    {
      return;
    }

    for (var row = 0; row < height; row++)
    {
      for (var column = 0; column < width; column++)
      {
        var byteIndex = row * bytesPerRow + (column / 8);
        var bitIndex = 7 - (column % 8);
        var on = (packedBitmap[byteIndex] & (1 << bitIndex)) != 0;
        if (on)
        {
          DrawPixel(x + column, y + row);
        }
      }
    }
  }

  public void DrawText(int x, int y, string text)
  {
    if (string.IsNullOrEmpty(text))
    {
      return;
    }

    var cursorX = x;
    foreach (var character in text)
    {
      var glyph = _font.GetGlyph(character);
      for (var row = 0; row < _font.GlyphHeight; row++)
      {
        var rowBits = glyph[row];
        for (var column = 0; column < _font.GlyphWidth; column++)
        {
          var bit = (rowBits >> (_font.GlyphWidth - 1 - column)) & 0x1;
          if (bit == 1)
          {
            DrawPixel(cursorX + column, y + row);
          }
        }
      }

      cursorX += _font.GlyphWidth + _font.CharSpacing;
    }
  }

  public Bitmap Render()
  {
    if (!_isDirty)
    {
      return _framebuffer;
    }

    for (var i = 0; i < _pixels.Length; i++)
    {
      _argb[i] = _pixels[i] == 1 ? unchecked((int)0xFFFFFFFF) : unchecked((int)0xFF000000);
    }

    var rect = new Rectangle(0, 0, Width, Height);
    var data = _framebuffer.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
    try
    {
      Marshal.Copy(_argb, 0, data.Scan0, _argb.Length);
    }
    finally
    {
      _framebuffer.UnlockBits(data);
    }

    _isDirty = false;
    return _framebuffer;
  }

  public void Render(Graphics graphics, Rectangle targetArea)
  {
    graphics.InterpolationMode = InterpolationMode.NearestNeighbor;
    graphics.PixelOffsetMode = PixelOffsetMode.Half;
    graphics.SmoothingMode = SmoothingMode.None;
    graphics.CompositingQuality = CompositingQuality.HighSpeed;
    graphics.Clear(Color.Black);

    graphics.DrawImage(Render(), targetArea);
  }

  public void Dispose()
  {
    _framebuffer.Dispose();
  }
}
