using System.Diagnostics;
using Aquarium.ControllerEmulator.Device;
using Aquarium.ControllerEmulator.Display;
using Aquarium.ControllerEmulator.Firmware;
using Aquarium.ControllerEmulator.Input;
using Aquarium.ControllerEmulator.UI;

namespace Aquarium.ControllerEmulator.Forms;

public sealed class MainForm : Form
{
  private static readonly byte[] LightIcon8x8 =
  [
    0b00111000,
    0b01000100,
    0b01000100,
    0b11111110,
    0b01000100,
    0b01000100,
    0b00111000,
    0b00000000
  ];

  private static readonly byte[] PumpIcon8x8 =
  [
    0b01111100,
    0b10000010,
    0b10111010,
    0b10111010,
    0b10111010,
    0b10000010,
    0b01111100,
    0b00000000
  ];

  private static readonly byte[] HeaterIcon8x8 =
  [
    0b00010000,
    0b00111000,
    0b00111000,
    0b00010000,
    0b01111100,
    0b00111000,
    0b00111000,
    0b00010000
  ];

  private static readonly byte[] FeederIcon8x8 =
  [
    0b00111100,
    0b01000010,
    0b10000001,
    0b10011001,
    0b10011001,
    0b10000001,
    0b01000010,
    0b00111100
  ];

  private readonly DeviceState _deviceState = new();
  private readonly OledRenderer _oledRenderer = new();
  private readonly FirmwareAnalyzer _firmwareAnalyzer = new();
  private readonly UiStateMachine _uiStateMachine;
  private readonly FirmwareAnalysisResult _analysis;
  private readonly InputController _inputController;
  private readonly System.Windows.Forms.Timer _frameTimer = new();
  private readonly Stopwatch _frameStopwatch = Stopwatch.StartNew();

  private readonly OledDisplayControl _oledDisplayControl = new();
  private readonly Label _analysisLabel = new();
  private readonly Label _statusLabel = new();
  private readonly Label _keyboardLabel = new();
  private readonly Button _upButton = new();
  private readonly Button _selectButton = new();
  private readonly Button _downButton = new();

  private long _lastTicks;

  public MainForm()
  {
    Text = "Aquarium.ControllerEmulator - Pixel OLED";
    StartPosition = FormStartPosition.CenterScreen;
    FormBorderStyle = FormBorderStyle.FixedDialog;
    MaximizeBox = false;
    MinimizeBox = false;
    ClientSize = new Size(800, 330);
    BackColor = Color.FromArgb(21, 27, 34);
    ForeColor = Color.White;
    Font = new Font("Segoe UI", 9f);

    var repositoryRoot = LocateRepositoryRoot() ?? Directory.GetCurrentDirectory();
    _analysis = _firmwareAnalyzer.Analyze(repositoryRoot);
    _uiStateMachine = new UiStateMachine(_analysis.UiModel);

    BuildLayout();

    _inputController = new InputController(this);
    _inputController.UpPressed += OnUpPressed;
    _inputController.SelectPressed += OnSelectPressed;
    _inputController.DownPressed += OnDownPressed;

    _frameTimer.Interval = 33; // 30 FPS
    _frameTimer.Tick += OnFrameTick;
    _lastTicks = _frameStopwatch.ElapsedTicks;
    _frameTimer.Start();

    RenderFrame();
  }

  protected override void OnFormClosed(FormClosedEventArgs e)
  {
    _frameTimer.Stop();
    _inputController.Dispose();
    _oledRenderer.Dispose();
    base.OnFormClosed(e);
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
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 536));
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
    Controls.Add(root);

    var leftPanel = new Panel { Dock = DockStyle.Fill, BackColor = BackColor };
    root.Controls.Add(leftPanel, 0, 0);

    _oledDisplayControl.Renderer = _oledRenderer;
    _oledDisplayControl.Location = new Point(0, 0);
    leftPanel.Controls.Add(_oledDisplayControl);
    CenterDisplay(leftPanel);
    leftPanel.Resize += (_, _) => CenterDisplay(leftPanel);

    var infoPanel = new Panel
    {
      Dock = DockStyle.Bottom,
      Height = 140,
      Padding = new Padding(0, 8, 0, 0),
      BackColor = BackColor
    };

    ConfigureInfoLabel(_analysisLabel);
    ConfigureInfoLabel(_statusLabel);
    ConfigureInfoLabel(_keyboardLabel);
    _keyboardLabel.Text = "Keyboard: UP=ArrowUp SELECT=Enter DOWN=ArrowDown";

    infoPanel.Controls.Add(_keyboardLabel);
    infoPanel.Controls.Add(_statusLabel);
    infoPanel.Controls.Add(_analysisLabel);

    _keyboardLabel.Dock = DockStyle.Top;
    _statusLabel.Dock = DockStyle.Top;
    _analysisLabel.Dock = DockStyle.Top;

    leftPanel.Controls.Add(infoPanel);

    var rightPanel = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 1,
      RowCount = 5,
      BackColor = BackColor
    };
    rightPanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
    rightPanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 70));
    rightPanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 70));
    rightPanel.RowStyles.Add(new RowStyle(SizeType.Absolute, 70));
    rightPanel.RowStyles.Add(new RowStyle(SizeType.Percent, 100f));
    root.Controls.Add(rightPanel, 1, 0);

    var titleLabel = new Label
    {
      Text = "Device buttons",
      Dock = DockStyle.Fill,
      ForeColor = Color.WhiteSmoke,
      TextAlign = ContentAlignment.BottomLeft
    };
    rightPanel.Controls.Add(titleLabel, 0, 0);

    ConfigureButton(_upButton, "UP", (_, _) => _inputController.PressUp());
    ConfigureButton(_selectButton, "SELECT", (_, _) => _inputController.PressSelect());
    ConfigureButton(_downButton, "DOWN", (_, _) => _inputController.PressDown());

    rightPanel.Controls.Add(_upButton, 0, 1);
    rightPanel.Controls.Add(_selectButton, 0, 2);
    rightPanel.Controls.Add(_downButton, 0, 3);
  }

  private void OnUpPressed()
  {
    _uiStateMachine.Up();
    RenderFrame();
  }

  private void OnSelectPressed()
  {
    _uiStateMachine.Select(_deviceState);
    RenderFrame();
  }

  private void OnDownPressed()
  {
    _uiStateMachine.Down(_deviceState);
    RenderFrame();
  }

  private void OnFrameTick(object? sender, EventArgs e)
  {
    var now = _frameStopwatch.ElapsedTicks;
    var elapsedSeconds = (double)(now - _lastTicks) / Stopwatch.Frequency;
    _lastTicks = now;

    _deviceState.Update(TimeSpan.FromSeconds(elapsedSeconds));
    _uiStateMachine.UpdateIdleTimeout(TimeSpan.FromSeconds(30));
    RenderFrame();
  }

  private void RenderFrame()
  {
    _oledRenderer.Clear();

    switch (_uiStateMachine.CurrentScreen)
    {
      case UiScreen.HomeScreen:
        DrawHomeScreen();
        break;
      case UiScreen.Menu:
      case UiScreen.Settings:
      case UiScreen.System:
      case UiScreen.SubMenu:
        DrawMenuScreen(_uiStateMachine.CurrentMenuTitle, _uiStateMachine.GetCurrentEntries(_deviceState), _uiStateMachine.CurrentSelection);
        break;
    }

    _analysisLabel.Text =
      $"Firmware scan: files={_analysis.SourceFiles.Count} states={_analysis.UiModel.States.Count} " +
      $"screens={_analysis.UiModel.Screens.Count} classes={_analysis.UiModel.CodeStructure.Classes.Count}";
    _statusLabel.Text =
      $"Screen: {_uiStateMachine.CurrentScreen}  Action: {_uiStateMachine.LastAction}  " +
      $"Temp: {_deviceState.Temperature:00.0}C  Feeder: {_deviceState.FeederState}";

    _oledDisplayControl.Invalidate();
  }

  private void DrawHomeScreen()
  {
    _oledRenderer.DrawRect(0, 0, OledRenderer.Width, OledRenderer.Height);
    _oledRenderer.DrawLine(0, 8, OledRenderer.Width - 1, 8);
    _oledRenderer.DrawLine(0, 22, OledRenderer.Width - 1, 22);

    _oledRenderer.DrawText(2, 1, _deviceState.CurrentTime.ToString("HH:mm:ss"));
    _oledRenderer.DrawText(72, 1, $"{_deviceState.Temperature:00.0}C");

    DrawStatusIcon(2, 11, LightIcon8x8, _deviceState.LightOn);
    DrawStatusIcon(34, 11, PumpIcon8x8, _deviceState.PumpOn);
    DrawStatusIcon(66, 11, HeaterIcon8x8, _deviceState.HeaterOn);
    DrawStatusIcon(98, 11, FeederIcon8x8, _deviceState.FeederState == FeederState.Feeding);

    _oledRenderer.DrawText(2, 24, $"BAT:{_deviceState.BatteryPercent:000}%");
    _oledRenderer.DrawText(68, 24, "SELECT=MENU");
  }

  private void DrawStatusIcon(int x, int y, ReadOnlySpan<byte> bitmap, bool active)
  {
    _oledRenderer.DrawRect(x, y, 28, 10);
    _oledRenderer.DrawBitmap(x + 2, y + 1, 8, 8, bitmap);
    if (active)
    {
      _oledRenderer.DrawRect(x + 18, y + 2, 8, 6, true);
    }
    else
    {
      _oledRenderer.DrawRect(x + 18, y + 2, 8, 6);
    }
  }

  private void DrawMenuScreen(string title, IReadOnlyList<string> entries, int selectedIndex)
  {
    _oledRenderer.DrawRect(0, 0, OledRenderer.Width, OledRenderer.Height);
    _oledRenderer.DrawLine(0, 8, OledRenderer.Width - 1, 8);
    _oledRenderer.DrawText(2, 1, TrimForOled(title, 14));
    _oledRenderer.DrawText(90, 1, "U S D");

    const int visibleRows = 3;
    var firstIndex = entries.Count <= visibleRows ? 0 : Math.Min(Math.Max(0, selectedIndex - (visibleRows - 1)), entries.Count - visibleRows);

    for (var row = 0; row < visibleRows; row++)
    {
      var index = firstIndex + row;
      if (index >= entries.Count)
      {
        continue;
      }

      var y = 10 + row * 7;
      if (index == selectedIndex)
      {
        _oledRenderer.DrawRect(1, y, 126, 7);
      }

      _oledRenderer.DrawText(4, y, TrimForOled(entries[index], 20));
    }
  }

  private static string TrimForOled(string value, int maxChars)
  {
    if (string.IsNullOrEmpty(value) || value.Length <= maxChars)
    {
      return value;
    }

    return value[..(maxChars - 1)] + "~";
  }

  private void CenterDisplay(Control host)
  {
    var x = Math.Max(0, (host.ClientSize.Width - _oledDisplayControl.Width) / 2);
    _oledDisplayControl.Location = new Point(x, 0);
  }

  private static void ConfigureInfoLabel(Label label)
  {
    label.ForeColor = Color.WhiteSmoke;
    label.AutoSize = false;
    label.Height = 28;
    label.TextAlign = ContentAlignment.MiddleLeft;
  }

  private static void ConfigureButton(Button button, string text, EventHandler onClick)
  {
    button.Text = text;
    button.Dock = DockStyle.Fill;
    button.FlatStyle = FlatStyle.Flat;
    button.FlatAppearance.BorderSize = 1;
    button.FlatAppearance.BorderColor = Color.FromArgb(80, 92, 106);
    button.BackColor = Color.FromArgb(35, 44, 54);
    button.ForeColor = Color.WhiteSmoke;
    button.TabStop = false;
    button.Click += onClick;
  }

  private string? LocateRepositoryRoot()
  {
    var current = new DirectoryInfo(AppContext.BaseDirectory);
    while (current is not null)
    {
      var firmwarePath = Path.Combine(current.FullName, "firmware", "src");
      if (Directory.Exists(firmwarePath))
      {
        return current.FullName;
      }

      current = current.Parent;
    }

    return null;
  }
}
