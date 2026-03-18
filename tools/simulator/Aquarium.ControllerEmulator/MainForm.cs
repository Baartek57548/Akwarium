namespace Aquarium.ControllerEmulator;

public sealed class MainForm : Form
{
  private readonly FirmwareBridge _bridge = new();
  private readonly OledRenderer _renderer = new();
  private readonly OledDisplayControl _display = new();
  private readonly System.Windows.Forms.Timer _timer = new() { Interval = 33 };
  private readonly Label _statusLabel = new();

  private NumericUpDown? _batteryPercentInput;
  private NumericUpDown? _aerationPercentInput;
  private NumericUpDown? _temperatureInput;
  private TextBox? _logMessageInput;
  private MaskedTextBox? _logTimeInput;

  public MainForm()
  {
    Text = "Aquarium.ControllerEmulator (Firmware UI)";
    StartPosition = FormStartPosition.CenterScreen;
    FormBorderStyle = FormBorderStyle.FixedSingle;
    MaximizeBox = false;
    MinimizeBox = false;
    BackColor = Color.FromArgb(18, 22, 28);
    ForeColor = Color.WhiteSmoke;
    ClientSize = new Size(1040, 420);
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
      Text = "Sterowanie: BACK=ArrowUp, SELECT=Enter, DOWN=ArrowDown",
      Padding = new Padding(0, 10, 0, 0)
    };
    left.Controls.Add(info);

    _statusLabel.Dock = DockStyle.Bottom;
    _statusLabel.Height = 42;
    _statusLabel.TextAlign = ContentAlignment.MiddleLeft;
    _statusLabel.ForeColor = Color.Silver;
    left.Controls.Add(_statusLabel);

    var right = new FlowLayoutPanel
    {
      Dock = DockStyle.Fill,
      AutoScroll = true,
      FlowDirection = FlowDirection.TopDown,
      WrapContents = false,
      BackColor = BackColor,
      Padding = new Padding(0),
      Margin = new Padding(0)
    };
    root.Controls.Add(right, 1, 0);

    var title = new Label
    {
      Width = 430,
      Height = 26,
      Text = "Przyciski urzadzenia",
      TextAlign = ContentAlignment.BottomLeft,
      ForeColor = Color.WhiteSmoke
    };
    right.Controls.Add(title);

    right.Controls.Add(CreateButton("BACK", () =>
    {
      _bridge.PressUp();
      RefreshFrame();
    }));

    right.Controls.Add(CreateButton("SELECT", () =>
    {
      _bridge.PressSelect();
      RefreshFrame();
    }));

    right.Controls.Add(CreateButton("DOWN", () =>
    {
      _bridge.PressDown();
      RefreshFrame();
    }));

    right.Controls.Add(CreateManualValuesGroup());
    right.Controls.Add(CreateLogsAndCalibrationGroup());
  }

  private GroupBox CreateManualValuesGroup()
  {
    var group = new GroupBox
    {
      Text = "Reczne parametry",
      Width = 430,
      Height = 170,
      ForeColor = Color.Gainsboro,
      BackColor = Color.FromArgb(24, 29, 36),
      Padding = new Padding(10)
    };

    var layout = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 3,
      RowCount = 4,
      BackColor = group.BackColor
    };
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 128));
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 120));
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
    group.Controls.Add(layout);

    _batteryPercentInput = new NumericUpDown
    {
      Minimum = 0,
      Maximum = 100,
      Value = 88,
      Width = 100
    };

    _aerationPercentInput = new NumericUpDown
    {
      Minimum = 0,
      Maximum = 100,
      Value = 30,
      Width = 100
    };

    _temperatureInput = new NumericUpDown
    {
      Minimum = 0,
      Maximum = 45,
      DecimalPlaces = 1,
      Increment = 0.1m,
      Value = 24.0m,
      Width = 100
    };

    layout.Controls.Add(CreateFieldLabel("Bateria [%]"), 0, 0);
    layout.Controls.Add(_batteryPercentInput, 1, 0);
    layout.Controls.Add(CreateFieldLabel("Napowietrzanie [%]"), 0, 1);
    layout.Controls.Add(_aerationPercentInput, 1, 1);
    layout.Controls.Add(CreateFieldLabel("Temperatura [C]"), 0, 2);
    layout.Controls.Add(_temperatureInput, 1, 2);

    var buttons = new FlowLayoutPanel
    {
      Dock = DockStyle.Fill,
      FlowDirection = FlowDirection.LeftToRight,
      WrapContents = false,
      BackColor = group.BackColor
    };

    var apply = CreateSmallButton("Zastosuj", ApplyManualValues);
    var auto = CreateSmallButton("Automatyczne", DisableManualValues);
    buttons.Controls.Add(apply);
    buttons.Controls.Add(auto);
    layout.Controls.Add(buttons, 0, 3);
    layout.SetColumnSpan(buttons, 3);

    return group;
  }

  private GroupBox CreateLogsAndCalibrationGroup()
  {
    var group = new GroupBox
    {
      Text = "Logi i kalibracja",
      Width = 430,
      Height = 190,
      ForeColor = Color.Gainsboro,
      BackColor = Color.FromArgb(24, 29, 36),
      Padding = new Padding(10)
    };

    var layout = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 3,
      RowCount = 4,
      BackColor = group.BackColor
    };
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 128));
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 180));
    layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
    layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
    group.Controls.Add(layout);

    _logMessageInput = new TextBox
    {
      Width = 168,
      MaxLength = 19,
      Text = "Manualny log"
    };

    _logTimeInput = new MaskedTextBox("00:00")
    {
      Width = 64,
      Text = DateTime.Now.ToString("HHmm")
    };

    layout.Controls.Add(CreateFieldLabel("Tresc logu"), 0, 0);
    layout.Controls.Add(_logMessageInput, 1, 0);
    layout.Controls.Add(CreateFieldLabel("Czas [HH:MM]"), 0, 1);
    layout.Controls.Add(_logTimeInput, 1, 1);

    var logButtons = new FlowLayoutPanel
    {
      Dock = DockStyle.Fill,
      FlowDirection = FlowDirection.LeftToRight,
      WrapContents = false,
      BackColor = group.BackColor
    };
    logButtons.Controls.Add(CreateSmallButton("Dodaj log", AddManualLogEntry));
    logButtons.Controls.Add(CreateSmallButton("Wyczysc logi", ClearManualLogs));
    layout.Controls.Add(logButtons, 0, 2);
    layout.SetColumnSpan(logButtons, 3);

    var calibrationButton = CreateSmallButton("Uruchom kalibracje", StartCalibrationAnimation);
    calibrationButton.Width = 160;
    layout.Controls.Add(calibrationButton, 0, 3);
    layout.SetColumnSpan(calibrationButton, 2);

    return group;
  }

  private static Label CreateFieldLabel(string text)
  {
    return new Label
    {
      Text = text,
      Dock = DockStyle.Fill,
      ForeColor = Color.Gainsboro,
      TextAlign = ContentAlignment.MiddleLeft
    };
  }

  private static Button CreateButton(string text, Action action)
  {
    var button = new Button
    {
      Text = text,
      Width = 430,
      Height = 54,
      FlatStyle = FlatStyle.Flat,
      BackColor = Color.FromArgb(35, 42, 50),
      ForeColor = Color.WhiteSmoke
    };
    button.FlatAppearance.BorderColor = Color.FromArgb(70, 80, 92);
    button.Click += (_, _) => action();
    return button;
  }

  private static Button CreateSmallButton(string text, Action action)
  {
    var button = new Button
    {
      Text = text,
      Height = 30,
      Width = 120,
      FlatStyle = FlatStyle.Flat,
      BackColor = Color.FromArgb(35, 42, 50),
      ForeColor = Color.WhiteSmoke
    };
    button.FlatAppearance.BorderColor = Color.FromArgb(70, 80, 92);
    button.Click += (_, _) => action();
    return button;
  }

  private void ApplyManualValues()
  {
    if (_batteryPercentInput is null || _aerationPercentInput is null || _temperatureInput is null)
    {
      return;
    }

    _bridge.SetManualBatteryPercent((int)_batteryPercentInput.Value);
    _bridge.SetManualAerationPercent((int)_aerationPercentInput.Value);
    _bridge.SetManualTemperature((float)_temperatureInput.Value);
    RefreshFrame();
  }

  private void DisableManualValues()
  {
    _bridge.SetManualBatteryPercent(-1);
    _bridge.SetManualAerationPercent(-1);
    _bridge.SetManualTemperature(float.NaN);
    RefreshFrame();
  }

  private void AddManualLogEntry()
  {
    if (_logMessageInput is null || _logTimeInput is null)
    {
      return;
    }

    var message = _logMessageInput.Text.Trim();
    if (string.IsNullOrWhiteSpace(message))
    {
      return;
    }

    var timeText = _logTimeInput.Text;
    if (!System.Text.RegularExpressions.Regex.IsMatch(timeText, @"^\d{2}:\d{2}$"))
    {
      timeText = DateTime.Now.ToString("HH:mm");
    }

    _bridge.AddManualLog(message, timeText);
    RefreshFrame();
  }

  private void ClearManualLogs()
  {
    _bridge.ClearManualLogs();
    RefreshFrame();
  }

  private void StartCalibrationAnimation()
  {
    _bridge.StartCalibrationAnimation();
    RefreshFrame();
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
      case Keys.Escape:
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
