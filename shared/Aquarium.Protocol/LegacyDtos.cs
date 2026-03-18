using System.Text.Json.Serialization;

namespace Aquarium.Protocol;

public enum AquariumScheduleMode
{
  Schedule = 0,
  AlwaysOn = 1,
  AlwaysOff = 2
}

public enum AquariumHeaterMode
{
  Threshold = 0,
  Off = 1
}

public sealed class AquariumStatus
{
  public static AquariumStatus Empty { get; } = new();

  [JsonPropertyName("tmp")]
  public double Temperature { get; init; } = double.NaN;

  [JsonPropertyName("tar")]
  public double TargetTemperature { get; init; }

  [JsonPropertyName("thr")]
  public double ThresholdTemperature { get; init; }

  [JsonPropertyName("hm")]
  public int HeaterModeCode { get; init; }

  [JsonPropertyName("hys")]
  public double TemperatureHysteresis { get; init; }

  [JsonPropertyName("mn")]
  public double MinimumTemperature { get; init; } = double.NaN;

  [JsonPropertyName("me")]
  public long MinimumTemperatureEpoch { get; init; }

  [JsonPropertyName("bv")]
  public double BatteryVoltage { get; init; }

  [JsonPropertyName("bp")]
  public int BatteryPercent { get; init; }

  [JsonPropertyName("l")]
  public bool IsLightOn { get; init; }

  [JsonPropertyName("f")]
  public bool IsFilterOn { get; init; }

  [JsonPropertyName("h")]
  public bool IsHeaterOn { get; init; }

  [JsonPropertyName("srv")]
  public int ServoPosition { get; init; }

  [JsonPropertyName("ip")]
  public string IpAddress { get; init; } = "-";

  [JsonPropertyName("ap")]
  public bool IsAccessPointMode { get; init; }

  [JsonPropertyName("cli")]
  public int ConnectedClients { get; init; }

  [JsonPropertyName("lm")]
  public int LightModeCode { get; init; }

  [JsonPropertyName("am")]
  public int AerationModeCode { get; init; }

  [JsonPropertyName("fm")]
  public int FilterModeCode { get; init; }

  [JsonIgnore]
  public AquariumHeaterMode HeaterMode => Enum.IsDefined(typeof(AquariumHeaterMode), HeaterModeCode)
    ? (AquariumHeaterMode)HeaterModeCode
    : AquariumHeaterMode.Threshold;

  [JsonIgnore]
  public AquariumScheduleMode LightMode => Enum.IsDefined(typeof(AquariumScheduleMode), LightModeCode)
    ? (AquariumScheduleMode)LightModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public AquariumScheduleMode AerationMode => Enum.IsDefined(typeof(AquariumScheduleMode), AerationModeCode)
    ? (AquariumScheduleMode)AerationModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public AquariumScheduleMode FilterMode => Enum.IsDefined(typeof(AquariumScheduleMode), FilterModeCode)
    ? (AquariumScheduleMode)FilterModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public double EffectiveThresholdTemperature => ThresholdTemperature > 0 ? ThresholdTemperature : TargetTemperature;

  public string TemperatureText => double.IsNaN(Temperature) || Temperature <= -99 ? "--.- C" : $"{Temperature:0.0} C";

  public string TargetTemperatureText => HeaterMode == AquariumHeaterMode.Off ? "OFF" : $"{EffectiveThresholdTemperature:0.0} C";

  public string BatteryText => $"{BatteryPercent}% ({BatteryVoltage:0.00} V)";

  public string BatteryVoltageText => $"{BatteryVoltage:0.00} V";

  public string FilterText => IsFilterOn ? "On" : "Off";

  public string LightText => IsLightOn ? "On" : "Off";

  public string HeaterText => HeaterMode == AquariumHeaterMode.Off ? "Disabled" : (IsHeaterOn ? "Heating" : "Standby");

  public string MinimumTemperatureText => double.IsNaN(MinimumTemperature) || MinimumTemperature <= -99
    ? "--.- C"
    : $"{MinimumTemperature:0.0} C";

  public string MinimumTemperatureTimeText => MinimumTemperatureEpoch <= 0
    ? "--:--"
    : DateTimeOffset.FromUnixTimeSeconds(MinimumTemperatureEpoch).ToLocalTime().ToString("dd.MM HH:mm");

  public int ServoOpenPercent => Math.Clamp((int)Math.Round((1d - (ServoPosition / 90d)) * 100d), 0, 100);

  public string ServoOpenPercentText => $"{ServoOpenPercent}%";

  public string ServoModeText => ServoPosition switch
  {
    <= 0 => "Open",
    >= 90 => "Closed",
    _ => "Partial"
  };

  public string NetworkBadgeText => IsAccessPointMode ? "ACCESS POINT" : "STATION";

  public string NetworkText => IsAccessPointMode ? $"AP {IpAddress}" : $"STA {IpAddress}";
}

public sealed class AquariumSettings
{
  [JsonPropertyName("lm")]
  public int LightModeCode { get; init; }

  [JsonPropertyName("tar")]
  public double TargetTemperature { get; init; }

  [JsonPropertyName("hm")]
  public int HeaterModeCode { get; init; }

  [JsonPropertyName("hys")]
  public double TemperatureHysteresis { get; init; }

  [JsonPropertyName("fdH")]
  public int FeedHour { get; init; }

  [JsonPropertyName("fdM")]
  public int FeedMinute { get; init; }

  [JsonPropertyName("fdF")]
  public int FeedMode { get; init; }

  [JsonPropertyName("lsH")]
  public int DayStartHour { get; init; }

  [JsonPropertyName("lsM")]
  public int DayStartMinute { get; init; }

  [JsonPropertyName("leH")]
  public int DayEndHour { get; init; }

  [JsonPropertyName("leM")]
  public int DayEndMinute { get; init; }

  [JsonPropertyName("am")]
  public int AerationModeCode { get; init; }

  [JsonPropertyName("asH")]
  public int AerationHourOn { get; init; }

  [JsonPropertyName("asM")]
  public int AerationMinuteOn { get; init; }

  [JsonPropertyName("aeH")]
  public int AerationHourOff { get; init; }

  [JsonPropertyName("aeM")]
  public int AerationMinuteOff { get; init; }

  [JsonPropertyName("fm")]
  public int FilterModeCode { get; init; }

  [JsonPropertyName("fsH")]
  public int FilterHourOn { get; init; }

  [JsonPropertyName("fsM")]
  public int FilterMinuteOn { get; init; }

  [JsonPropertyName("feH")]
  public int FilterHourOff { get; init; }

  [JsonPropertyName("feM")]
  public int FilterMinuteOff { get; init; }

  [JsonPropertyName("spO")]
  public int ServoPreOffMinutes { get; init; }

  [JsonIgnore]
  public AquariumScheduleMode LightMode => Enum.IsDefined(typeof(AquariumScheduleMode), LightModeCode)
    ? (AquariumScheduleMode)LightModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public AquariumScheduleMode AerationMode => Enum.IsDefined(typeof(AquariumScheduleMode), AerationModeCode)
    ? (AquariumScheduleMode)AerationModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public AquariumScheduleMode FilterMode => Enum.IsDefined(typeof(AquariumScheduleMode), FilterModeCode)
    ? (AquariumScheduleMode)FilterModeCode
    : AquariumScheduleMode.Schedule;

  [JsonIgnore]
  public AquariumHeaterMode HeaterMode => Enum.IsDefined(typeof(AquariumHeaterMode), HeaterModeCode)
    ? (AquariumHeaterMode)HeaterModeCode
    : AquariumHeaterMode.Threshold;

  public Dictionary<string, object> ToBlePayload()
  {
    var payload = new Dictionary<string, object>
    {
      ["lm"] = LightModeCode,
      ["am"] = AerationModeCode,
      ["fm"] = FilterModeCode,
      ["hm"] = HeaterModeCode,
      ["hys"] = TemperatureHysteresis,
      ["fdH"] = FeedHour,
      ["fdM"] = FeedMinute,
      ["fdF"] = FeedMode
    };

    if (LightMode == AquariumScheduleMode.Schedule)
    {
      payload["lsH"] = DayStartHour;
      payload["lsM"] = DayStartMinute;
      payload["leH"] = DayEndHour;
      payload["leM"] = DayEndMinute;
    }

    if (AerationMode == AquariumScheduleMode.Schedule)
    {
      payload["asH"] = AerationHourOn;
      payload["asM"] = AerationMinuteOn;
      payload["aeH"] = AerationHourOff;
      payload["aeM"] = AerationMinuteOff;
    }

    if (FilterMode == AquariumScheduleMode.Schedule)
    {
      payload["fsH"] = FilterHourOn;
      payload["fsM"] = FilterMinuteOn;
      payload["feH"] = FilterHourOff;
      payload["feM"] = FilterMinuteOff;
    }

    if (HeaterMode == AquariumHeaterMode.Threshold)
    {
      payload["tar"] = TargetTemperature;
    }

    return payload;
  }
}

public sealed class AquariumDeviceInfo
{
  public static AquariumDeviceInfo Empty { get; } = new();

  [JsonPropertyName("nm")]
  public string FirmwareName { get; init; } = "Aquarium Controller";

  [JsonPropertyName("ver")]
  public string FirmwareVersion { get; init; } = "-";

  [JsonPropertyName("dt")]
  public string BuildDate { get; init; } = "-";

  [JsonPropertyName("tm")]
  public string BuildTime { get; init; } = "-";

  [JsonPropertyName("idf")]
  public string IdfVersion { get; init; } = "-";

  [JsonPropertyName("rp")]
  public string RunningPartition { get; init; } = "-";

  [JsonPropertyName("bp")]
  public string BootPartition { get; init; } = "-";

  [JsonPropertyName("np")]
  public string NextPartition { get; init; } = "-";

  [JsonPropertyName("slot")]
  public int OtaPartitionSizeBytes { get; init; }

  [JsonPropertyName("busy")]
  public bool IsOtaInProgress { get; init; }

  [JsonPropertyName("ota")]
  public string ActiveOtaTransport { get; init; } = "idle";

  [JsonPropertyName("ble")]
  public bool SupportsBleOta { get; init; }

  [JsonPropertyName("http")]
  public bool SupportsHttpOta { get; init; }

  [JsonPropertyName("chunk")]
  public int RecommendedChunkSizeBytes { get; init; }

  [JsonPropertyName("val")]
  public AquariumValidationProfile ValidationProfile { get; init; } = AquariumValidationProfile.Default;

  public string FirmwareDisplayText => string.IsNullOrWhiteSpace(FirmwareVersion) ? "-" : FirmwareVersion;

  public string BuildDisplayText => $"{BuildDate} {BuildTime}".Trim();

  public string OtaSlotSizeText => OtaPartitionSizeBytes <= 0
    ? "-"
    : $"{OtaPartitionSizeBytes / 1024d / 1024d:0.00} MB";

  public string OtaTransportText => string.Equals(ActiveOtaTransport, "idle", StringComparison.OrdinalIgnoreCase)
    ? "Idle"
    : ActiveOtaTransport.ToUpperInvariant();

  public string CompatibilitySummary => SupportsBleOta
    ? "BLE OTA available"
    : "BLE OTA unavailable";
}

public sealed class AquariumValidationProfile
{
  public static AquariumValidationProfile Default { get; } = new();

  [JsonPropertyName("ms")]
  public int MinuteStep { get; init; } = 5;

  [JsonPropertyName("tmp")]
  public AquariumNumericRule Temperature { get; init; } = new()
  {
    Min = 18,
    Max = 30,
    Step = 1,
    SupportsOff = true
  };

  [JsonPropertyName("hys")]
  public AquariumNumericRule Hysteresis { get; init; } = new()
  {
    Min = 0.1,
    Max = 5.0,
    Step = 0.1
  };

  [JsonPropertyName("fd")]
  public AquariumIntegerRule FeedModes { get; init; } = new()
  {
    Min = 0,
    Max = 3
  };
}

public sealed class AquariumNumericRule
{
  [JsonPropertyName("min")]
  public double Min { get; init; }

  [JsonPropertyName("max")]
  public double Max { get; init; }

  [JsonPropertyName("step")]
  public double Step { get; init; }

  [JsonPropertyName("off")]
  public bool SupportsOff { get; init; }
}

public sealed class AquariumIntegerRule
{
  [JsonPropertyName("min")]
  public int Min { get; init; }

  [JsonPropertyName("max")]
  public int Max { get; init; }
}

public sealed class AquariumCommand
{
  private AquariumCommand(string action, int? angle)
  {
    Action = action;
    Angle = angle;
  }

  public string Action { get; }

  public int? Angle { get; }

  public static AquariumCommand FeedNow() => new("feed_now", null);

  public static AquariumCommand SetServo(int angle) => new("set_servo", Math.Clamp(angle, 0, 90));

  public static AquariumCommand ClearServo() => new("clear_servo", null);

  public static AquariumCommand ClearCriticalLogs() => new("clear_critical_logs", null);

  public AquariumCommandPayload ToPayload() => new(Action, Angle);
}

public sealed class AquariumCommandPayload
{
  public AquariumCommandPayload(string action, int? angle)
  {
    Action = action;
    Angle = angle;
  }

  [JsonPropertyName("action")]
  public string Action { get; }

  [JsonPropertyName("angle")]
  [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
  public int? Angle { get; }
}
