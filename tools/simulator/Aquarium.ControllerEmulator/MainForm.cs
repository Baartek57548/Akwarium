namespace Aquarium.ControllerEmulator;

public sealed class MainForm : Form
{
  private readonly FirmwareBridge _bridge = new();
  private readonly OledRenderer _renderer = new();
  private readonly OledDisplayControl _display = new();
  private readonly System.Windows.Forms.Timer _timer = new() { Interval = 33 };
  private readonly Label _statusLabel = new();

  public MainForm()
  {
    Text = "Aquarium.ControllerEmulator (Firmware UI)";
    StartPosition = FormStartPosition.CenterScreen;
    FormBorderStyle = FormBorderStyle.FixedSingle;
    MaximizeBox = false;
    MinimizeBox = false;
    BackColor = Color.FromArgb(18, 22, 28);
    ForeColor = Color.WhiteSmoke;
    ClientSize = new Size(760, 300);
    KeyPreview = true;

    BuildLayout();

    _timer.Tick += (_, _) => RefreshFrame();
    KeyDown += OnMainFormKeyDown;
    PreviewKeyDown += OnMainFormPreviewKeyDown;
    FormClosed += OnMainFormClosed;
    Load += OnMainFormLoad;
  }

  private void BuildLayout()
  {
    var root = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 2,
      RowCount = 1,
      Padding = new Padding(16),
      BackColor = BackColor
    };
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 540));
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
    Controls.Add(root);

    var left = new Panel { Dock = DockStyle.Fill, BackColor = BackColor };
    root.Controls.Add(left, 0, 0);

    _display.Renderer = _renderer;
    _display.Location = new Point(0, 0);
    left.Controls.Add(_display);
    left.Resize += (_, _) => CenterDisplay(left);
    CenterDisplay(left);

    var info = new Label
    {
      Dock = DockStyle.Bottom,
      Height = 42,
      TextAlign = ContentAlignment.MiddleLeft,
      ForeColor = Color.Gainsboro,
      Text = "Sterowanie: UP=ArrowUp, SELECT=Enter, DOWN=ArrowDown",
      Padding = new Padding(0, 10, 0, 0)
    };
    left.Controls.Add(info);

    _statusLabel.Dock = DockStyle.Bottom;
    _statusLabel.Height = 42;
    _statusLabel.TextAlign = ContentAlignment.MiddleLeft;
    _statusLabel.ForeColor = Color.Silver;
    left.Controls.Add(_statusLabel);

    var right = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 1,
      RowCount = 6,
      BackColor = BackColor
    };
    right.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    right.RowStyles.Add(new RowStyle(SizeType.Absolute, 64));
    right.RowStyles.Add(new RowStyle(SizeType.Absolute, 64));
    right.RowStyles.Add(new RowStyle(SizeType.Absolute, 64));
    right.RowStyles.Add(new RowStyle(SizeType.Absolute, 64));
    right.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
    root.Controls.Add(right, 1, 0);

    var title = new Label
    {
      Dock = DockStyle.Fill,
      Text = "Przyciski urzadzenia",
      TextAlign = ContentAlignment.BottomLeft,
      ForeColor = Color.WhiteSmoke
    };
    right.Controls.Add(title, 0, 0);

    right.Controls.Add(CreateButton("UP", () =>
    {
      _bridge.PressUp();
      RefreshFrame();
    }), 0, 1);

    right.Controls.Add(CreateButton("SELECT", () =>
    {
      _bridge.PressSelect();
      RefreshFrame();
    }), 0, 2);

    right.Controls.Add(CreateButton("DOWN", () =>
    {
      _bridge.PressDown();
      RefreshFrame();
    }), 0, 3);
  }

  private static Button CreateButton(string text, Action action)
  {
    var button = new Button
    {
      Text = text,
      Dock = DockStyle.Fill,
      Height = 54,
      FlatStyle = FlatStyle.Flat,
      BackColor = Color.FromArgb(35, 42, 50),
      ForeColor = Color.WhiteSmoke
    };
    button.FlatAppearance.BorderColor = Color.FromArgb(70, 80, 92);
    button.Click += (_, _) => action();
    return button;
  }

  private void CenterDisplay(Control container)
  {
    _display.Left = (container.ClientSize.Width - _display.Width) / 2;
    _display.Top = 2;
  }

  private void OnMainFormLoad(object? sender, EventArgs e)
  {
    if (!_bridge.Initialize(out var error))
    {
      _statusLabel.Text = error;
      return;
    }

    _statusLabel.Text = "FirmwareUI.dll zaladowane. Emulacja aktywna (30 FPS).";
    _timer.Start();
    RefreshFrame();
  }

  private void OnMainFormClosed(object? sender, FormClosedEventArgs e)
  {
    _timer.Stop();
    _bridge.Dispose();
    _renderer.Dispose();
  }

  private void OnMainFormKeyDown(object? sender, KeyEventArgs e)
  {
    switch (e.KeyCode)
    {
      case Keys.Up:
        _bridge.PressUp();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
      case Keys.Enter:
        _bridge.PressSelect();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
      case Keys.Down:
        _bridge.PressDown();
        e.Handled = true;
        e.SuppressKeyPress = true;
        break;
      default:
        return;
    }

    RefreshFrame();
  }

  private static void OnMainFormPreviewKeyDown(object? sender, PreviewKeyDownEventArgs e)
  {
    if (e.KeyCode is Keys.Up or Keys.Down)
    {
      e.IsInputKey = true;
    }
  }

  private void RefreshFrame()
  {
    var frame = _bridge.GetFrameBuffer();
    _renderer.UpdateFromNative(frame);
    _display.Invalidate();
  }
}
