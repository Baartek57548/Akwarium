namespace Aquarium.ControllerEmulator.Input;

public sealed class InputController : IDisposable
{
  private readonly Form _host;

  public InputController(Form host)
  {
    _host = host;
    _host.KeyPreview = true;
    _host.PreviewKeyDown += HostOnPreviewKeyDown;
    _host.KeyDown += HostOnKeyDown;
  }

  public event Action? UpPressed;
  public event Action? SelectPressed;
  public event Action? DownPressed;

  public void PressUp()
  {
    UpPressed?.Invoke();
  }

  public void PressSelect()
  {
    SelectPressed?.Invoke();
  }

  public void PressDown()
  {
    DownPressed?.Invoke();
  }

  public void Dispose()
  {
    _host.PreviewKeyDown -= HostOnPreviewKeyDown;
    _host.KeyDown -= HostOnKeyDown;
  }

  private static void HostOnPreviewKeyDown(object? sender, PreviewKeyDownEventArgs e)
  {
    if (e.KeyCode is Keys.Up or Keys.Down)
    {
      e.IsInputKey = true;
    }
  }

  private void HostOnKeyDown(object? sender, KeyEventArgs e)
  {
    switch (e.KeyCode)
    {
      case Keys.Up:
        PressUp();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
      case Keys.Enter:
        PressSelect();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
      case Keys.Down:
        PressDown();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
    }
  }
}
