using System.Collections.Concurrent;
using Aquarium.Models;
using Aquarium.Protocol;

namespace Aquarium.EmulatorCore;

public sealed class EmulatedDeviceCore : IDeviceController
{
  private readonly object _gate = new();
  private readonly ConcurrentQueue<string> _log = new();
  private DeviceStatus _status;
  private DeviceConfig _config;
  private SystemInfo _systemInfo;
  private bool _running;
  private bool _sensorError;
  private bool _feederJam;
  private bool _otaBusy;
  private bool _isConnected;
  private DateTimeOffset _bootTime;
  private DateTimeOffset _clock;

  public event EventHandler? StateChanged;

  public EmulatedDeviceCore()
  {
    _clock = DateTimeOffset.Now;
    _bootTime = _clock;
    _config = CreateDefaultConfig();
    _systemInfo = CreateDefaultSystemInfo();
    _status = new DeviceStatus
    {
      Temperature = new TemperatureData
      {
        Current = _config.Temperature.Target,
        Minimum = _config.Temperature.Target,
        Maximum = _config.Temperature.Target,
        Target = _config.Temperature.Target,
        Hysteresis = _config.Temperature.Hysteresis,
        SensorAvailable = true,
        HeaterEnabled = true
      },
      Relays = new RelayState
      {
        Heater = true,
        Filter = false,
        Light = false,
        DayMode = false,
        ServoAngle = 90,
        AerationPercent = 0
      },
      Feeder = new FeederStatus
      {
        IsFeeding = false,
        IsJammed = false,
        Mode = _config.FeedMode,
        LastFeedEpoch = 0,
        LastErrorCode = "none",
        SafetyTimeoutSeconds = 15
      },
      System = _systemInfo,
      Config = _config,
      Clock = _clock,
      BatteryVoltage = 3.95,
      BatteryPercent = 84
    };
    _status = CreateStatusSnapshot();
    _running = true;
    PushLog("info", "Emulator initialized.");
  }

  public bool IsRunning
  {
    get
    {
      lock (_gate)
      {
        return _running;
      }
    }
  }

  public bool IsConnected => _isConnected;

  public IReadOnlyCollection<string> Logs => _log.ToArray();

  public void Start()
  {
    lock (_gate)
    {
      if (_running)
      {
        return;
      }

      _running = true;
      _bootTime = _clock;
      _systemInfo = _systemInfo with { UptimeSeconds = 0 };
      PushLog("info", "Device started.");
      RaiseStateChanged();
    }
  }

  public void Stop()
  {
    lock (_gate)
    {
      if (!_running)
      {
        return;
      }

      _running = false;
      _status = _status with
      {
        Relays = new RelayState
        {
          Heater = false,
          Filter = false,
          Light = false,
          DayMode = false,
          ServoAngle = _status.Relays.ServoAngle,
          AerationPercent = 0
        }
      };
      PushLog("info", "Device stopped.");
      RaiseStateChanged();
    }
  }

  public void SetTemperature(double temperature)
  {
    lock (_gate)
    {
      _sensorError = false;
      var telemetry = _status.Temperature with
      {
        Current = temperature,
        SensorAvailable = true,
        Minimum = double.IsNaN(_status.Temperature.Minimum) || temperature < _status.Temperature.Minimum
          ? temperature
          : _status.Temperature.Minimum,
        Maximum = double.IsNaN(_status.Temperature.Maximum) || temperature > _status.Temperature.Maximum
          ? temperature
          : _status.Temperature.Maximum
      };

      _status = _status with { Temperature = telemetry };
      PushLog("info", $"Temperature set to {temperature:0.0} C.");
      RaiseStateChanged();
    }
  }

  public void SimulateSensorError(bool enabled)
  {
    lock (_gate)
    {
      _sensorError = enabled;
      _status = _status with
      {
        Temperature = _status.Temperature with
        {
          SensorAvailable = !enabled,
          Current = enabled ? double.NaN : _status.Temperature.Current
        }
      };
      PushLog("warn", enabled ? "DS18B20 sensor error enabled." : "DS18B20 sensor recovered.");
      RaiseStateChanged();
    }
  }

  public void SimulateFeederJam(bool enabled)
  {
    lock (_gate)
    {
      _feederJam = enabled;
      _status = _status with
      {
        Feeder = _status.Feeder with
        {
          IsJammed = enabled,
          LastErrorCode = enabled ? "feeder_jam" : "none"
        }
      };
      PushLog("warn", enabled ? "Feeder jam simulated." : "Feeder jam cleared.");
      RaiseStateChanged();
    }
  }

  public void AdvanceTime(TimeSpan delta)
  {
    lock (_gate)
    {
      _clock += delta;
      if (_running)
      {
        _systemInfo = _systemInfo with { UptimeSeconds = (long)(_clock - _bootTime).TotalSeconds };
      }
      RaiseStateChanged();
    }
  }

  public Task<DeviceStatus> ReadStatusAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      return Task.FromResult(_status);
    }
  }

  public Task<DeviceConfig> ReadSettingsAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      return Task.FromResult(_config);
    }
  }

  public Task<SystemInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      return Task.FromResult(_systemInfo);
    }
  }

  public Task<DeviceResponse> SendCommandAsync(DeviceCommand command, CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      return Task.FromResult(ApplyCommand(command));
    }
  }

  public Task<DeviceResponse> SaveSettingsAsync(DeviceConfig settings, CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      _config = settings;
      _status = _status with
      {
        Config = settings,
        Temperature = _status.Temperature with
        {
          Target = settings.Temperature.Target,
          Hysteresis = settings.Temperature.Hysteresis
        }
      };
      PushLog("info", "Settings stored in emulator memory.");
      RaiseStateChanged();
      return Task.FromResult(new DeviceResponse(true, "settings_saved", "Settings stored in emulator memory."));
    }
  }

  public Task<DeviceResponse> StartOtaAsync(OtaRequest request, CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      if (_otaBusy)
      {
        return Task.FromResult(new DeviceResponse(false, "ota_busy", "An OTA session is already active."));
      }

      _otaBusy = true;
      _systemInfo = _systemInfo with { IsOtaInProgress = true, ActiveOtaTransport = request.Action };
      PushLog("info", $"OTA started using {request.Action}.");
      RaiseStateChanged();
      return Task.FromResult(new DeviceResponse(true, "ota_ready", "OTA session initialized."));
    }
  }

  public Task<DeviceResponse> FinishOtaAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      if (!_otaBusy)
      {
        return Task.FromResult(new DeviceResponse(false, "ota_not_started", "No active OTA session."));
      }

      _otaBusy = false;
      _systemInfo = _systemInfo with { IsOtaInProgress = false, ActiveOtaTransport = "idle" };
      PushLog("info", "OTA finished.");
      RaiseStateChanged();
      return Task.FromResult(new DeviceResponse(true, "ota_complete", "OTA finished successfully."));
    }
  }

  public Task<DeviceResponse> AbortOtaAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      if (!_otaBusy)
      {
        return Task.FromResult(new DeviceResponse(false, "ota_not_started", "No active OTA session."));
      }

      _otaBusy = false;
      _systemInfo = _systemInfo with { IsOtaInProgress = false, ActiveOtaTransport = "idle" };
      PushLog("warn", "OTA aborted.");
      RaiseStateChanged();
      return Task.FromResult(new DeviceResponse(true, "ota_aborted", "OTA aborted."));
    }
  }

  public Task ConnectAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      _isConnected = true;
      PushLog("info", "Emulated connection established.");
      RaiseStateChanged();
    }

    return Task.CompletedTask;
  }

  public Task DisconnectAsync(CancellationToken cancellationToken = default)
  {
    cancellationToken.ThrowIfCancellationRequested();
    lock (_gate)
    {
      _isConnected = false;
      PushLog("info", "Emulated connection closed.");
      RaiseStateChanged();
    }

    return Task.CompletedTask;
  }

  public DeviceStatus Snapshot()
  {
    lock (_gate)
    {
      return _status;
    }
  }

  public SystemInfo SnapshotSystemInfo()
  {
    lock (_gate)
    {
      return _systemInfo;
    }
  }

  private DeviceResponse ApplyCommand(DeviceCommand command)
  {
    switch (command.Action)
    {
      case "feed_now":
        _status = _status with
        {
          Feeder = _status.Feeder with
          {
            IsFeeding = true,
            LastFeedEpoch = _clock.ToUnixTimeSeconds(),
            IsJammed = _feederJam,
            LastErrorCode = _feederJam ? "feeder_jam" : "none"
          }
        };
        PushLog("info", "Feed command executed.");
        RaiseStateChanged();
        return new DeviceResponse(true, "feed_now", "Feeder command executed.");
      case "set_servo":
        _status = _status with
        {
          Relays = _status.Relays with
          {
            ServoAngle = Math.Clamp(command.Angle ?? 0, 0, 90)
          }
        };
        PushLog("info", $"Servo angle set to {command.Angle ?? 0}.");
        RaiseStateChanged();
        return new DeviceResponse(true, "set_servo", "Servo angle updated.");
      case "clear_servo":
        _status = _status with
        {
          Relays = _status.Relays with
          {
            ServoAngle = 90
          }
        };
        PushLog("info", "Servo override cleared.");
        RaiseStateChanged();
        return new DeviceResponse(true, "clear_servo", "Servo override cleared.");
      case "clear_critical_logs":
        ClearCriticalLogs();
        PushLog("info", "Critical logs cleared.");
        RaiseStateChanged();
        return new DeviceResponse(true, "clear_critical_logs", "Critical logs cleared.");
      default:
        return new DeviceResponse(false, "unknown_action", $"Unknown command: {command.Action}");
    }
  }

  private void ClearCriticalLogs()
  {
    var remaining = _log.Where(entry => !entry.Contains("[warn]", StringComparison.OrdinalIgnoreCase) &&
                                        !entry.Contains("[error]", StringComparison.OrdinalIgnoreCase))
      .ToArray();

    while (_log.TryDequeue(out _))
    {
    }

    foreach (var entry in remaining)
    {
      _log.Enqueue(entry);
    }
  }

  private void PushLog(string level, string message)
  {
    var entry = $"[{DateTimeOffset.Now:HH:mm:ss}] [{level}] {message}";
    _log.Enqueue(entry);
    while (_log.Count > 250 && _log.TryDequeue(out _))
    {
    }
  }

  private void RaiseStateChanged()
  {
    _systemInfo = _systemInfo with { UptimeSeconds = (long)(_clock - _bootTime).TotalSeconds };
    _status = CreateStatusSnapshot();
    StateChanged?.Invoke(this, EventArgs.Empty);
  }

  private DeviceConfig CreateDefaultConfig()
  {
    return new DeviceConfig
    {
      Lighting = new Schedule
      {
        Mode = ScheduleMode.Schedule,
        StartHour = 10,
        StartMinute = 0,
        EndHour = 21,
        EndMinute = 30
      },
      Aeration = new Schedule
      {
        Mode = ScheduleMode.Schedule,
        StartHour = 10,
        StartMinute = 0,
        EndHour = 19,
        EndMinute = 0
      },
      Filter = new Schedule
      {
        Mode = ScheduleMode.Schedule,
        StartHour = 10,
        StartMinute = 30,
        EndHour = 20,
        EndMinute = 30,
        PreOffMinutes = 30
      },
      Temperature = new TemperatureData
      {
        Current = 24.0,
        Minimum = 24.0,
        Maximum = 24.0,
        Target = 25.0,
        Hysteresis = 0.5,
        SensorAvailable = true
      },
      HeaterMode = HeaterMode.Threshold,
      FeedHour = 18,
      FeedMinute = 0,
      FeedMode = 1,
      ServoPreOffMinutes = 30
    };
  }

  private SystemInfo CreateDefaultSystemInfo()
  {
    return new SystemInfo
    {
      FirmwareName = "Aquarium Emulator",
      FirmwareVersion = "0.1.0",
      BuildRef = "local-emulator",
      BuildDate = DateTimeOffset.Now.ToString("yyyy-MM-dd"),
      BuildTime = DateTimeOffset.Now.ToString("HH:mm:ss"),
      IdfVersion = "-",
      RunningPartition = "emulator",
      BootPartition = "emulator",
      NextPartition = "emulator",
      OtaPartitionSizeBytes = 1024 * 1024 * 2,
      IsOtaInProgress = false,
      ActiveOtaTransport = "idle",
      SupportsBleOta = true,
      SupportsHttpOta = true,
      RecommendedChunkSizeBytes = 160,
      IpAddress = "127.0.0.1",
      IsAccessPointMode = false,
      ConnectedClients = 0
    };
  }

  private DeviceStatus CreateStatusSnapshot()
  {
    return new DeviceStatus
    {
      Temperature = _sensorError
        ? new TemperatureData
        {
          Current = double.NaN,
          Minimum = _status.Temperature.Minimum,
          Maximum = _status.Temperature.Maximum,
          Target = _config.Temperature.Target,
          Hysteresis = _config.Temperature.Hysteresis,
          SensorAvailable = false,
          HeaterEnabled = false
        }
        : new TemperatureData
        {
          Current = _status.Temperature.Current,
          Minimum = _status.Temperature.Minimum,
          Maximum = _status.Temperature.Maximum,
          Target = _config.Temperature.Target,
          Hysteresis = _config.Temperature.Hysteresis,
          SensorAvailable = true,
          HeaterEnabled = _status.Temperature.HeaterEnabled
        },
      Relays = new RelayState
      {
        Heater = _status.Relays.Heater,
        Filter = _status.Relays.Filter,
        Light = _status.Relays.Light,
        DayMode = _status.Relays.DayMode,
        ServoAngle = _status.Relays.ServoAngle,
        AerationPercent = _status.Relays.AerationPercent
      },
      Feeder = _status.Feeder,
      System = _systemInfo,
      Config = _config,
      Clock = _clock,
      BatteryVoltage = _status.BatteryVoltage,
      BatteryPercent = _status.BatteryPercent
    };
  }
}
