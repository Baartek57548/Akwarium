using Aquarium.Models;

namespace Aquarium.Protocol;

public static class LegacyModelMappers
{
  public static DeviceStatus ToDeviceStatus(this AquariumStatus status)
  {
    return new DeviceStatus
    {
      Temperature = new TemperatureData
      {
        Current = status.Temperature,
        Minimum = status.MinimumTemperature,
        MinimumAt = status.MinimumTemperatureEpoch > 0
          ? DateTimeOffset.FromUnixTimeSeconds(status.MinimumTemperatureEpoch)
          : null,
        Target = status.TargetTemperature,
        Hysteresis = status.TemperatureHysteresis,
        SensorAvailable = !double.IsNaN(status.Temperature) && status.Temperature > -99,
        HeaterEnabled = status.IsHeaterOn
      },
      Relays = new RelayState
      {
        Heater = status.IsHeaterOn,
        Filter = status.IsFilterOn,
        Light = status.IsLightOn,
        DayMode = status.ConnectedClients >= 0,
        ServoAngle = status.ServoPosition
      },
      Feeder = new FeederStatus
      {
        IsFeeding = false,
        LastFeedEpoch = 0,
        LastErrorCode = "none",
        SafetyTimeoutSeconds = 15
      },
      System = new SystemInfo
      {
        FirmwareName = "Aquarium Controller",
        FirmwareVersion = "-",
        IpAddress = status.IpAddress,
        IsAccessPointMode = status.IsAccessPointMode,
        ConnectedClients = status.ConnectedClients
      },
      Config = new DeviceConfig
      {
        HeaterMode = status.IsHeaterOn ? HeaterMode.Threshold : HeaterMode.Off,
        Temperature = new TemperatureData
        {
          Target = status.TargetTemperature,
          Hysteresis = status.TemperatureHysteresis
        }
      },
      BatteryVoltage = status.BatteryVoltage,
      BatteryPercent = status.BatteryPercent,
      Clock = DateTimeOffset.MinValue
    };
  }

  public static DeviceConfig ToDeviceConfig(this AquariumSettings settings)
  {
    return new DeviceConfig
    {
      Lighting = new Schedule
      {
        Mode = (ScheduleMode)settings.LightModeCode,
        StartHour = settings.DayStartHour,
        StartMinute = settings.DayStartMinute,
        EndHour = settings.DayEndHour,
        EndMinute = settings.DayEndMinute
      },
      Aeration = new Schedule
      {
        Mode = (ScheduleMode)settings.AerationModeCode,
        StartHour = settings.AerationHourOn,
        StartMinute = settings.AerationMinuteOn,
        EndHour = settings.AerationHourOff,
        EndMinute = settings.AerationMinuteOff
      },
      Filter = new Schedule
      {
        Mode = (ScheduleMode)settings.FilterModeCode,
        StartHour = settings.FilterHourOn,
        StartMinute = settings.FilterMinuteOn,
        EndHour = settings.FilterHourOff,
        EndMinute = settings.FilterMinuteOff,
        PreOffMinutes = settings.ServoPreOffMinutes
      },
      Temperature = new TemperatureData
      {
        Target = settings.TargetTemperature,
        Hysteresis = settings.TemperatureHysteresis
      },
      HeaterMode = settings.HeaterModeCode == (int)AquariumHeaterMode.Off ? HeaterMode.Off : HeaterMode.Threshold,
      FeedHour = settings.FeedHour,
      FeedMinute = settings.FeedMinute,
      FeedMode = settings.FeedMode,
      ServoPreOffMinutes = settings.ServoPreOffMinutes
    };
  }

  public static SystemInfo ToSystemInfo(this AquariumDeviceInfo info)
  {
    return new SystemInfo
    {
      FirmwareName = info.FirmwareName,
      FirmwareVersion = info.FirmwareVersion,
      BuildRef = "-",
      BuildDate = info.BuildDate,
      BuildTime = info.BuildTime,
      IdfVersion = info.IdfVersion,
      RunningPartition = info.RunningPartition,
      BootPartition = info.BootPartition,
      NextPartition = info.NextPartition,
      OtaPartitionSizeBytes = info.OtaPartitionSizeBytes,
      IsOtaInProgress = info.IsOtaInProgress,
      ActiveOtaTransport = info.ActiveOtaTransport,
      SupportsBleOta = info.SupportsBleOta,
      SupportsHttpOta = info.SupportsHttpOta,
      RecommendedChunkSizeBytes = info.RecommendedChunkSizeBytes
    };
  }

  public static AquariumStatus ToLegacyStatus(this DeviceStatus status)
  {
    return new AquariumStatus
    {
      Temperature = status.Temperature.Current,
      TargetTemperature = status.Temperature.Target,
      ThresholdTemperature = status.Temperature.Target + status.Temperature.Hysteresis,
      HeaterModeCode = status.Config.HeaterMode == HeaterMode.Off
        ? (int)AquariumHeaterMode.Off
        : (int)AquariumHeaterMode.Threshold,
      TemperatureHysteresis = status.Temperature.Hysteresis,
      MinimumTemperature = status.Temperature.Minimum,
      MinimumTemperatureEpoch = status.Temperature.MinimumAt?.ToUnixTimeSeconds() ?? 0,
      BatteryVoltage = status.BatteryVoltage,
      BatteryPercent = status.BatteryPercent,
      IsLightOn = status.Relays.Light,
      IsFilterOn = status.Relays.Filter,
      IsHeaterOn = status.Relays.Heater,
      ServoPosition = status.Relays.ServoAngle,
      IpAddress = status.System.IpAddress,
      IsAccessPointMode = status.System.IsAccessPointMode,
      ConnectedClients = status.System.ConnectedClients,
      LightModeCode = (int)status.Config.Lighting.Mode,
      AerationModeCode = (int)status.Config.Aeration.Mode,
      FilterModeCode = (int)status.Config.Filter.Mode
    };
  }

  public static AquariumSettings ToLegacySettings(this DeviceConfig config)
  {
    return new AquariumSettings
    {
      LightModeCode = (int)config.Lighting.Mode,
      TargetTemperature = config.Temperature.Target,
      HeaterModeCode = config.HeaterMode == HeaterMode.Off
        ? (int)AquariumHeaterMode.Off
        : (int)AquariumHeaterMode.Threshold,
      TemperatureHysteresis = config.Temperature.Hysteresis,
      FeedHour = config.FeedHour,
      FeedMinute = config.FeedMinute,
      FeedMode = config.FeedMode,
      DayStartHour = config.Lighting.StartHour,
      DayStartMinute = config.Lighting.StartMinute,
      DayEndHour = config.Lighting.EndHour,
      DayEndMinute = config.Lighting.EndMinute,
      AerationModeCode = (int)config.Aeration.Mode,
      AerationHourOn = config.Aeration.StartHour,
      AerationMinuteOn = config.Aeration.StartMinute,
      AerationHourOff = config.Aeration.EndHour,
      AerationMinuteOff = config.Aeration.EndMinute,
      FilterModeCode = (int)config.Filter.Mode,
      FilterHourOn = config.Filter.StartHour,
      FilterMinuteOn = config.Filter.StartMinute,
      FilterHourOff = config.Filter.EndHour,
      FilterMinuteOff = config.Filter.EndMinute,
      ServoPreOffMinutes = config.ServoPreOffMinutes
    };
  }

  public static AquariumDeviceInfo ToLegacyDeviceInfo(this SystemInfo info)
  {
    return new AquariumDeviceInfo
    {
      FirmwareName = info.FirmwareName,
      FirmwareVersion = info.FirmwareVersion,
      BuildDate = info.BuildDate,
      BuildTime = info.BuildTime,
      IdfVersion = info.IdfVersion,
      RunningPartition = info.RunningPartition,
      BootPartition = info.BootPartition,
      NextPartition = info.NextPartition,
      OtaPartitionSizeBytes = info.OtaPartitionSizeBytes,
      IsOtaInProgress = info.IsOtaInProgress,
      ActiveOtaTransport = info.ActiveOtaTransport,
      SupportsBleOta = info.SupportsBleOta,
      SupportsHttpOta = info.SupportsHttpOta,
      RecommendedChunkSizeBytes = info.RecommendedChunkSizeBytes
    };
  }
}
