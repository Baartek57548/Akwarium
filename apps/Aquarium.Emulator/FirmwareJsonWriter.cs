using System.Text.Json;
using Aquarium.EmulatorCore;
using Aquarium.Models;
using Aquarium.Protocol;

namespace Aquarium.Emulator;

internal static class FirmwareJsonWriter
{
  private static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web)
  {
    DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull
  };

  public static string BuildStatusJson(DeviceStatus status)
  {
    var payload = new
    {
      temperature = new
      {
        current = double.IsNaN(status.Temperature.Current) ? -99.9 : status.Temperature.Current,
        target = status.Temperature.Target,
        threshold = status.Temperature.Target + status.Temperature.Hysteresis,
        heaterMode = status.Config.HeaterMode == Aquarium.Models.HeaterMode.Off ? 1 : 0,
        hysteresis = status.Temperature.Hysteresis,
        min = double.IsNaN(status.Temperature.Minimum) ? -99.9 : status.Temperature.Minimum,
        minTimeEpoch = status.Temperature.MinimumAt?.ToUnixTimeSeconds() ?? 0
      },
      battery = new
      {
        voltage = status.BatteryVoltage,
        percent = status.BatteryPercent
      },
      relays = new
      {
        light = status.Relays.Light,
        pump = status.Relays.Filter,
        heater = status.Relays.Heater
      },
      servo = new
      {
        angle = status.Relays.ServoAngle
      },
      schedule = new
      {
        lightMode = (int)status.Config.Lighting.Mode,
        dayStartHour = status.Config.Lighting.StartHour,
        dayStartMin = status.Config.Lighting.StartMinute,
        dayEndHour = status.Config.Lighting.EndHour,
        dayEndMin = status.Config.Lighting.EndMinute,
        airMode = (int)status.Config.Aeration.Mode,
        airStartHour = status.Config.Aeration.StartHour,
        airStartMin = status.Config.Aeration.StartMinute,
        airEndHour = status.Config.Aeration.EndHour,
        airEndMin = status.Config.Aeration.EndMinute,
        filterMode = (int)status.Config.Filter.Mode,
        filterStartHour = status.Config.Filter.StartHour,
        filterStartMin = status.Config.Filter.StartMinute,
        filterEndHour = status.Config.Filter.EndHour,
        filterEndMin = status.Config.Filter.EndMinute,
        heaterMode = status.Config.HeaterMode == Aquarium.Models.HeaterMode.Off ? 1 : 0,
        servoPreOffMins = status.Config.ServoPreOffMinutes
      },
      feeding = new
      {
        hour = status.Config.FeedHour,
        minute = status.Config.FeedMinute,
        freq = status.Config.FeedMode,
        lastFeedEpoch = status.Feeder.LastFeedEpoch
      },
      network = new
      {
        ip = status.System.IpAddress,
        apMode = status.System.IsAccessPointMode,
        ssid = status.System.IsAccessPointMode ? "AquariumAP" : "WiFi",
        clients = status.System.ConnectedClients
      },
      system = new
      {
        firmwareName = status.System.FirmwareName,
        firmwareVersion = status.System.FirmwareVersion,
        buildRef = status.System.BuildRef,
        buildDate = status.System.BuildDate,
        buildTime = status.System.BuildTime,
        idfVersion = status.System.IdfVersion,
        runningPartition = status.System.RunningPartition,
        bootPartition = status.System.BootPartition,
        nextPartition = status.System.NextPartition,
        otaPartitionSize = status.System.OtaPartitionSizeBytes,
        otaInProgress = status.System.IsOtaInProgress,
        otaTransport = status.System.ActiveOtaTransport,
        bleOtaSupported = status.System.SupportsBleOta,
        httpOtaSupported = status.System.SupportsHttpOta,
        uptimeSec = status.System.UptimeSeconds,
        resetReason = "emulator",
        validation = new
        {
          minuteStep = 5,
          scheduleModes = "schedule|always_on|always_off",
          heaterModes = "threshold|off",
          timeFieldsRequireScheduleMode = true,
          temperature = new { min = 18, max = 30, step = 1, supportsOff = true },
          hysteresis = new { min = 0.1, max = 5.0, step = 0.1 },
          servoPreOffMinutes = new { min = 0, max = 120, step = 1 },
          feeding = new { modeMin = 0, modeMax = 3 }
        }
      },
      clock = new
      {
        hour = status.Clock.Hour,
        minute = status.Clock.Minute,
        second = status.Clock.Second,
        day = status.Clock.Day,
        month = status.Clock.Month,
        year = status.Clock.Year
      }
    };

    return JsonSerializer.Serialize(payload, Options);
  }

  public static string BuildSettingsJson(DeviceConfig config)
  {
    var legacy = config.ToLegacySettings();
    return JsonSerializer.Serialize(legacy, Options);
  }

  public static string BuildDeviceInfoJson(SystemInfo info)
  {
    var legacy = info.ToLegacyDeviceInfo();
    return JsonSerializer.Serialize(legacy, Options);
  }
}
