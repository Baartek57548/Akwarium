using Aquarium.Models;

namespace Aquarium.Protocol;

public static class WebApiMappers
{
  public static WebStatusResponse ToWebStatusResponse(this DeviceStatus status)
  {
    ArgumentNullException.ThrowIfNull(status);

    var config = status.Config ?? new DeviceConfig();
    var temperature = status.Temperature ?? new TemperatureData();
    var relays = status.Relays ?? new RelayState();
    var feeder = status.Feeder ?? new FeederStatus();
    var system = status.System ?? new SystemInfo();
    var clock = status.Clock;

    return new WebStatusResponse(
      WebApiContract.SchemaVersion,
      new WebTemperatureSnapshot(
        double.IsNaN(temperature.Current) ? -99.9d : temperature.Current,
        temperature.Target,
        temperature.Target + temperature.Hysteresis,
        config.HeaterMode == HeaterMode.Off ? 1 : 0,
        temperature.Hysteresis,
        double.IsNaN(temperature.Minimum) ? -99.9d : temperature.Minimum,
        temperature.MinimumAt?.ToUnixTimeSeconds() ?? 0,
        10,
        0,
        []),
      new WebBatterySnapshot(status.BatteryVoltage, status.BatteryPercent),
      new WebRelaySnapshot(relays.Light, relays.Filter, relays.Heater, relays.AerationPercent),
      new WebScheduleSnapshot(
        (int)config.Lighting.Mode,
        config.Lighting.StartHour,
        config.Lighting.StartMinute,
        config.Lighting.EndHour,
        config.Lighting.EndMinute,
        (int)config.Aeration.Mode,
        config.Aeration.StartHour,
        config.Aeration.StartMinute,
        config.Aeration.EndHour,
        config.Aeration.EndMinute,
        (int)config.Filter.Mode,
        config.Filter.StartHour,
        config.Filter.StartMinute,
        config.Filter.EndHour,
        config.Filter.EndMinute,
        config.HeaterMode == HeaterMode.Off ? 1 : 0,
        config.ServoPreOffMinutes),
      new WebFeedingSnapshot(
        config.FeedHour,
        config.FeedMinute,
        config.FeedMode,
        feeder.LastFeedEpoch,
        feeder.IsFeeding),
      new WebNetworkSnapshot(
        system.IpAddress,
        system.IsAccessPointMode,
        system.IsAccessPointMode ? "AquariumAP" : "WiFi",
        system.ConnectedClients,
        !system.IsAccessPointMode,
        false,
        system.IsAccessPointMode,
        string.Empty,
        string.Empty,
        string.Empty,
        0,
        false,
        0,
        false,
        "Brak danych synchronizacji czasu.",
        false,
        false,
        false),
      new WebClockSnapshot(clock.Hour, clock.Minute, clock.Second, clock.Day, clock.Month, clock.Year),
      new WebFirmwareSnapshot(
        system.FirmwareName,
        system.FirmwareVersion,
        system.BuildRef,
        system.BuildDate,
        system.BuildTime,
        system.IdfVersion));
  }

  public static WebLogsResponse ToWebLogsResponse(
    this IEnumerable<string> logs,
    IEnumerable<string>? criticalLogs = null)
  {
    ArgumentNullException.ThrowIfNull(logs);

    var normal = new List<string>();
    var critical = new List<string>();

    foreach (var entry in logs)
    {
      if (string.IsNullOrWhiteSpace(entry))
      {
        continue;
      }

      if (entry.Contains("[warn]", StringComparison.OrdinalIgnoreCase) ||
          entry.Contains("[error]", StringComparison.OrdinalIgnoreCase))
      {
        critical.Add(entry);
      }
      else
      {
        normal.Add(entry);
      }
    }

    if (criticalLogs is not null)
    {
      critical = criticalLogs.Where(static entry => !string.IsNullOrWhiteSpace(entry)).ToList();
    }

    return new WebLogsResponse(normal, critical);
  }

  public static WebActionResponse ToWebActionResponse(this DeviceResponse response)
  {
    ArgumentNullException.ThrowIfNull(response);
    return new WebActionResponse(response.Success, response.Code, response.Message);
  }
}
