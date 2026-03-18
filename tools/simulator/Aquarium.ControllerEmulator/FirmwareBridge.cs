using System.Runtime.InteropServices;

namespace Aquarium.ControllerEmulator;

public sealed class FirmwareBridge : IDisposable
{
  private const int FrameBufferSize = OledRenderer.Width * OledRenderer.Height;

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate int InitUiDelegate();

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate void ButtonDelegate();

  [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
  private delegate IntPtr FrameBufferDelegate();

  private IntPtr _libraryHandle;
  private InitUiDelegate? _initUi;
  private ButtonDelegate? _pressUp;
  private ButtonDelegate? _pressDown;
  private ButtonDelegate? _pressSelect;
  private FrameBufferDelegate? _getFrameBuffer;
  private readonly byte[] _managedFrameBuffer = new byte[FrameBufferSize];
  private bool _isInitialized;

  public bool Initialize(out string error)
  {
    error = string.Empty;

    if (_isInitialized)
    {
      return true;
    }

    var candidates = GetLibraryCandidates();
    foreach (var candidate in candidates)
    {
      if (!File.Exists(candidate))
      {
        continue;
      }

      try
      {
        _libraryHandle = NativeLibrary.Load(candidate);
      }
      catch
      {
        _libraryHandle = IntPtr.Zero;
      }

      if (_libraryHandle != IntPtr.Zero)
      {
        break;
      }
    }

    if (_libraryHandle == IntPtr.Zero)
    {
      error = "Nie znaleziono FirmwareUI.dll. Zbuduj tools/simulator/FirmwareUI.";
      return false;
    }

    try
    {
      _initUi = GetExport<InitUiDelegate>("initUI");
      _pressUp = GetExport<ButtonDelegate>("pressButtonUp");
      _pressDown = GetExport<ButtonDelegate>("pressButtonDown");
      _pressSelect = GetExport<ButtonDelegate>("pressButtonSelect");
      _getFrameBuffer = GetExport<FrameBufferDelegate>("getFrameBuffer");
    }
    catch (Exception ex)
    {
      error = $"Nieprawidlowe eksporty DLL: {ex.Message}";
      return false;
    }

    if (_initUi() == 0)
    {
      error = "initUI() zwrocilo blad.";
      return false;
    }

    _isInitialized = true;
    return true;
  }

  public void PressUp()
  {
    _pressUp?.Invoke();
  }

  public void PressDown()
  {
    _pressDown?.Invoke();
  }

  public void PressSelect()
  {
    _pressSelect?.Invoke();
  }

  public ReadOnlySpan<byte> GetFrameBuffer()
  {
    if (_getFrameBuffer is null)
    {
      return _managedFrameBuffer;
    }

    var pointer = _getFrameBuffer();
    if (pointer == IntPtr.Zero)
    {
      return _managedFrameBuffer;
    }

    Marshal.Copy(pointer, _managedFrameBuffer, 0, _managedFrameBuffer.Length);
    return _managedFrameBuffer;
  }

  public void Dispose()
  {
    if (_libraryHandle != IntPtr.Zero)
    {
      NativeLibrary.Free(_libraryHandle);
      _libraryHandle = IntPtr.Zero;
    }
  }

  private T GetExport<T>(string name) where T : Delegate
  {
    var address = NativeLibrary.GetExport(_libraryHandle, name);
    return Marshal.GetDelegateForFunctionPointer<T>(address);
  }

  private static IEnumerable<string> GetLibraryCandidates()
  {
    var paths = new List<string>();

    var fromEnv = Environment.GetEnvironmentVariable("AQUARIUM_FIRMWARE_UI_DLL");
    if (!string.IsNullOrWhiteSpace(fromEnv))
    {
      paths.Add(Path.GetFullPath(fromEnv));
    }

    var baseDir = AppContext.BaseDirectory;
    paths.Add(Path.Combine(baseDir, "FirmwareUI.dll"));
    paths.Add(Path.Combine(baseDir, "native", "FirmwareUI.dll"));
    paths.Add(Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "..", "FirmwareUI", "build", "FirmwareUI.dll")));
    paths.Add(Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "..", "FirmwareUI", "build", "Debug", "FirmwareUI.dll")));
    paths.Add(Path.GetFullPath(Path.Combine(baseDir, "..", "..", "..", "..", "FirmwareUI", "build", "Release", "FirmwareUI.dll")));

    return paths.Distinct(StringComparer.OrdinalIgnoreCase);
  }
}
