namespace Aquarium.Models;

public enum ScheduleMode : byte
{
  Schedule = 0,
  AlwaysOn = 1,
  AlwaysOff = 2
}

public enum HeaterMode : byte
{
  Threshold = 0,
  Off = 1
}

public sealed record class TemperatureData
{
  public double Current { get; init; } = double.NaN;

  public double Minimum { get; init; } = double.NaN;

  public DateTimeOffset? MinimumAt { get; init; }

  public double Maximum { get; init; } = double.NaN;

  public double Target { get; init; }

  public double Hysteresis { get; init; }

  public bool SensorAvailable { get; init; }

  public bool HeaterEnabled { get; init; }
}

public sealed record class RelayState
{
  public bool Heater { get; init; }

  public bool Filter { get; init; }

  public bool Light { get; init; }

  public bool DayMode { get; init; }

  public int AerationPercent { get; init; }

  public int ServoAngle { get; init; }
}

public sealed record class Schedule
{
  public ScheduleMode Mode { get; init; } = ScheduleMode.Schedule;

  public int StartHour { get; init; }

  public int StartMinute { get; init; }

  public int EndHour { get; init; }

  public int EndMinute { get; init; }

  public int? PreOffMinutes { get; init; }

  public bool UsesWindow => Mode == ScheduleMode.Schedule;
}

public sealed record class FeederStatus
{
  public bool IsFeeding { get; init; }

  public bool IsJammed { get; init; }

  public int Mode { get; init; }

  public long LastFeedEpoch { get; init; }

  public string LastErrorCode { get; init; } = "none";

  public int SafetyTimeoutSeconds { get; init; }
}

public sealed record class SystemInfo
{
  public string FirmwareName { get; init; } = "Aquarium Controller";

  public string FirmwareVersion { get; init; } = "-";

  public string BuildRef { get; init; } = "local";

  public string BuildDate { get; init; } = "-";

  public string BuildTime { get; init; } = "-";

  public string IdfVersion { get; init; } = "-";

  public string RunningPartition { get; init; } = "-";

  public string BootPartition { get; init; } = "-";

  public string NextPartition { get; init; } = "-";

  public int OtaPartitionSizeBytes { get; init; }

  public bool IsOtaInProgress { get; init; }

  public string ActiveOtaTransport { get; init; } = "idle";

  public bool SupportsBleOta { get; init; }

  public bool SupportsHttpOta { get; init; }

  public int RecommendedChunkSizeBytes { get; init; } = 160;

  public string IpAddress { get; init; } = "-";

  public bool IsAccessPointMode { get; init; }

  public int ConnectedClients { get; init; }

  public long UptimeSeconds { get; init; }
}

public sealed record class DeviceConfig
{
  public Schedule Lighting { get; init; } = new();

  public Schedule Aeration { get; init; } = new();

  public Schedule Filter { get; init; } = new();

  public TemperatureData Temperature { get; init; } = new();

  public HeaterMode HeaterMode { get; init; } = HeaterMode.Threshold;

  public int FeedHour { get; init; }

  public int FeedMinute { get; init; }

  public int FeedMode { get; init; }

  public int ServoPreOffMinutes { get; init; }

  public int ServoDayAngle { get; init; } = 0;

  public int ServoNightAngle { get; init; } = 90;

  public int ServoAlarmAngle { get; init; } = 45;

  public bool AlwaysScreenOn { get; init; }
}

public sealed record class DeviceStatus
{
  public TemperatureData Temperature { get; init; } = new();

  public RelayState Relays { get; init; } = new();

  public FeederStatus Feeder { get; init; } = new();

  public SystemInfo System { get; init; } = new();

  public DeviceConfig Config { get; init; } = new();

  public DateTimeOffset Clock { get; init; } = DateTimeOffset.MinValue;

  public double BatteryVoltage { get; init; }

  public int BatteryPercent { get; init; }
}
