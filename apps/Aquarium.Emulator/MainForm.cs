using Aquarium.EmulatorCore;
using Aquarium.Protocol;

namespace Aquarium.Emulator;

internal sealed class MainForm : Form
{
  private readonly EmulatedDeviceCore _core = new();
  private readonly EmulatorHttpApiServer _httpApi;
  private readonly System.Windows.Forms.Timer _uiTimer = new();
  private readonly Label _stateLabel = new();
  private readonly Label _temperatureLabel = new();
  private readonly Label _relaysLabel = new();
  private readonly Label _wifiLabel = new();
  private readonly Label _uptimeLabel = new();
  private readonly Label _sensorLabel = new();
  private readonly Label _rtcLabel = new();
  private readonly Label _batteryLabel = new();
  private readonly Label _httpLabel = new();
  private readonly TextBox _logBox = new();
  private readonly Button _startButton = new();
  private readonly Button _stopButton = new();
  private readonly Button _sensorErrorButton = new();
  private readonly Button _tempUpButton = new();
  private readonly Button _tempDownButton = new();
  private readonly Button _jamButton = new();
  private readonly Button _httpStartButton = new();
  private readonly Button _httpStopButton = new();
  private bool _sensorErrorEnabled;
  private bool _feederJamEnabled;
  private int _lastLogCount;

  public MainForm()
  {
    Text = "Aquarium Emulator";
    Width = 1280;
    Height = 860;
    BackColor = Color.FromArgb(16, 24, 39);
    ForeColor = Color.WhiteSmoke;
    Font = new Font("Segoe UI", 10f);

    _httpApi = new EmulatorHttpApiServer(_core, 5080);

    BuildLayout();

    _core.StateChanged += (_, _) => RefreshView();
    _uiTimer.Interval = 1000;
    _uiTimer.Tick += (_, _) => _core.AdvanceTime(TimeSpan.FromSeconds(1));
    _uiTimer.Start();
    _ = StartHttpApiAsync();

    RefreshView();
  }

  protected override async void OnFormClosing(FormClosingEventArgs e)
  {
    _uiTimer.Stop();
    await _httpApi.StopAsync();
    base.OnFormClosing(e);
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
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 420));
    root.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100f));
    Controls.Add(root);

    root.Controls.Add(BuildLeftColumn(), 0, 0);
    root.Controls.Add(BuildRightColumn(), 1, 0);
  }

  private Control BuildLeftColumn()
  {
    var stack = new FlowLayoutPanel
    {
      Dock = DockStyle.Fill,
      FlowDirection = FlowDirection.TopDown,
      WrapContents = false,
      AutoScroll = true,
      BackColor = BackColor
    };

    stack.Controls.Add(BuildCard("Device State", _stateLabel, _temperatureLabel, _relaysLabel, _wifiLabel, _uptimeLabel));
    stack.Controls.Add(BuildCard("Sensors", _sensorLabel, _rtcLabel, _batteryLabel, _httpLabel));
    stack.Controls.Add(BuildControlCard());
    return stack;
  }

  private Control BuildRightColumn()
  {
    var panel = new Panel { Dock = DockStyle.Fill, BackColor = BackColor, Padding = new Padding(0, 0, 0, 0) };
    var group = new GroupBox
    {
      Dock = DockStyle.Fill,
      Text = "Logs",
      ForeColor = Color.WhiteSmoke,
      BackColor = Color.FromArgb(15, 23, 42)
    };

    _logBox.Dock = DockStyle.Fill;
    _logBox.Multiline = true;
    _logBox.ReadOnly = true;
    _logBox.ScrollBars = ScrollBars.Vertical;
    _logBox.BackColor = Color.FromArgb(2, 8, 20);
    _logBox.ForeColor = Color.FromArgb(103, 232, 249);
    _logBox.Font = new Font("Consolas", 9f);

    group.Controls.Add(_logBox);
    panel.Controls.Add(group);
    return panel;
  }

  private Control BuildCard(string title, params Control[] items)
  {
    var group = new GroupBox
    {
      Text = title,
      ForeColor = Color.WhiteSmoke,
      BackColor = Color.FromArgb(15, 23, 42),
      Width = 392,
      Height = 160,
      Margin = new Padding(0, 0, 0, 14)
    };

    var inner = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 1,
      RowCount = items.Length,
      Padding = new Padding(12)
    };

    foreach (var item in items)
    {
      item.Dock = DockStyle.Fill;
      inner.RowStyles.Add(new RowStyle(SizeType.AutoSize));
      inner.Controls.Add(item);
    }

    group.Controls.Add(inner);
    return group;
  }

  private Control BuildControlCard()
  {
    var group = new GroupBox
    {
      Text = "Controls",
      ForeColor = Color.WhiteSmoke,
      BackColor = Color.FromArgb(15, 23, 42),
      Width = 392,
      Height = 220,
      Margin = new Padding(0, 0, 0, 14)
    };

    var grid = new TableLayoutPanel
    {
      Dock = DockStyle.Fill,
      ColumnCount = 2,
      RowCount = 4,
      Padding = new Padding(12)
    };
    grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50f));
    grid.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50f));

    ConfigureButton(_startButton, "Start device", (_, _) =>
    {
      _core.Start();
      AppendLog("Device started.");
    });
    ConfigureButton(_stopButton, "Stop device", (_, _) =>
    {
      _core.Stop();
      AppendLog("Device stopped.");
    });
    ConfigureButton(_sensorErrorButton, "Simulate sensor error", (_, _) =>
    {
      _sensorErrorEnabled = !_sensorErrorEnabled;
      _core.SimulateSensorError(_sensorErrorEnabled);
      AppendLog(_sensorErrorEnabled ? "Sensor error enabled." : "Sensor error cleared.");
    });
    ConfigureButton(_tempUpButton, "Temperature +1C", (_, _) =>
    {
      var snapshot = _core.Snapshot();
      _core.SetTemperature((double.IsNaN(snapshot.Temperature.Current) ? snapshot.Temperature.Target : snapshot.Temperature.Current) + 1.0);
      AppendLog("Temperature increased.");
    });
    ConfigureButton(_tempDownButton, "Temperature -1C", (_, _) =>
    {
      var snapshot = _core.Snapshot();
      _core.SetTemperature((double.IsNaN(snapshot.Temperature.Current) ? snapshot.Temperature.Target : snapshot.Temperature.Current) - 1.0);
      AppendLog("Temperature decreased.");
    });
    ConfigureButton(_jamButton, "Simulate feeder jam", (_, _) =>
    {
      _feederJamEnabled = !_feederJamEnabled;
      _core.SimulateFeederJam(_feederJamEnabled);
      AppendLog(_feederJamEnabled ? "Feeder jam enabled." : "Feeder jam cleared.");
    });
    ConfigureButton(_httpStartButton, "Start HTTP API", async (_, _) =>
    {
      await StartHttpApiAsync();
    });
    ConfigureButton(_httpStopButton, "Stop HTTP API", async (_, _) =>
    {
      await StopHttpApiAsync();
    });

    grid.Controls.Add(_startButton, 0, 0);
    grid.Controls.Add(_stopButton, 1, 0);
    grid.Controls.Add(_sensorErrorButton, 0, 1);
    grid.Controls.Add(_tempUpButton, 1, 1);
    grid.Controls.Add(_tempDownButton, 0, 2);
    grid.Controls.Add(_jamButton, 1, 2);
    grid.Controls.Add(_httpStartButton, 0, 3);
    grid.Controls.Add(_httpStopButton, 1, 3);

    group.Controls.Add(grid);
    return group;
  }

  private static void ConfigureButton(Button button, string text, EventHandler handler)
  {
    button.Text = text;
    button.Height = 34;
    button.Dock = DockStyle.Fill;
    button.BackColor = Color.FromArgb(59, 130, 246);
    button.ForeColor = Color.White;
    button.FlatStyle = FlatStyle.Flat;
    button.FlatAppearance.BorderSize = 0;
    button.Margin = new Padding(4);
    button.Click += handler;
  }

  private void RefreshView()
  {
    var status = _core.Snapshot();
    var system = status.System;

    _stateLabel.Text = $"State: {( _core.IsRunning ? "RUNNING" : "STOPPED" )}";
    _temperatureLabel.Text = $"Temperature: {FormatTemperature(status.Temperature.Current)} | target {status.Temperature.Target:0.0} C";
    _relaysLabel.Text = $"Relays: heater {(status.Relays.Heater ? "ON" : "OFF")}, filter {(status.Relays.Filter ? "ON" : "OFF")}, light {(status.Relays.Light ? "ON" : "OFF")}, servo {status.Relays.ServoAngle} deg";
    _wifiLabel.Text = $"WiFi: {(system.IsAccessPointMode ? "AP" : "STA")} {system.IpAddress} | clients {system.ConnectedClients}";
    _uptimeLabel.Text = $"Uptime: {TimeSpan.FromSeconds(system.UptimeSeconds):hh\\:mm\\:ss}";
    _sensorLabel.Text = $"DS18B20: {(status.Temperature.SensorAvailable ? "OK" : "ERROR")}, heater {(status.Temperature.HeaterEnabled ? "ON" : "OFF")}, jam {(status.Feeder.IsJammed ? "YES" : "NO")}";
    _rtcLabel.Text = $"RTC: {status.Clock:yyyy-MM-dd HH:mm:ss}";
    _batteryLabel.Text = $"Battery: {status.BatteryPercent}% ({status.BatteryVoltage:0.00} V)";
    _httpLabel.Text = $"HTTP API: {(_httpApi.IsRunning ? $"http://127.0.0.1:{_httpApi.Port}/" : "stopped")}";

    _sensorErrorButton.Text = _sensorErrorEnabled ? "Clear sensor error" : "Simulate sensor error";
    _jamButton.Text = _feederJamEnabled ? "Clear feeder jam" : "Simulate feeder jam";
    _httpStartButton.Enabled = !_httpApi.IsRunning;
    _httpStopButton.Enabled = _httpApi.IsRunning;
    SyncLogs();
  }

  private void AppendLog(string message)
  {
    var line = $"[{DateTime.Now:HH:mm:ss}] {message}";
    _logBox.AppendText(line + Environment.NewLine);
    _logBox.SelectionStart = _logBox.Text.Length;
    _logBox.ScrollToCaret();
  }

  private void SyncLogs()
  {
    var logs = _core.Logs.ToArray();
    if (_lastLogCount > logs.Length)
    {
      _lastLogCount = 0;
    }

    for (var i = _lastLogCount; i < logs.Length; i++)
    {
      _logBox.AppendText(logs[i] + Environment.NewLine);
    }

    if (logs.Length > _lastLogCount)
    {
      _logBox.SelectionStart = _logBox.Text.Length;
      _logBox.ScrollToCaret();
    }

    _lastLogCount = logs.Length;
  }

  private async Task StartHttpApiAsync()
  {
    try
    {
      await _httpApi.StartAsync();
      AppendLog($"HTTP API started on http://127.0.0.1:{_httpApi.Port}/");
    }
    catch (Exception ex)
    {
      AppendLog($"HTTP API start failed: {ex.Message}");
    }
    finally
    {
      RefreshView();
    }
  }

  private async Task StopHttpApiAsync()
  {
    try
    {
      await _httpApi.StopAsync();
      AppendLog("HTTP API stopped.");
    }
    catch (Exception ex)
    {
      AppendLog($"HTTP API stop failed: {ex.Message}");
    }
    finally
    {
      RefreshView();
    }
  }

  private static string FormatTemperature(double value)
  {
    return double.IsNaN(value) ? "--.- C" : $"{value:0.0} C";
  }
}
