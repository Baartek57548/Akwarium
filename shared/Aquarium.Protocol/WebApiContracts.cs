namespace Aquarium.Protocol;

public static class WebApiContract
{
  public const string SchemaVersion = "2026-04-01";
}

public sealed record WebTemperatureHistoryPoint(double Value, long Epoch);

public sealed record WebTemperatureSnapshot(
  double Current,
  double Target,
  double Threshold,
  int HeaterMode,
  double Hysteresis,
  double Min,
  long MinTimeEpoch,
  int HistoryIntervalMinutes,
  int HistoryCapacity,
  IReadOnlyList<WebTemperatureHistoryPoint> History);

public sealed record WebBatterySnapshot(double Voltage, int Percent);

public sealed record WebRelaySnapshot(bool Light, bool Pump, bool Heater, int AerationPercent);

public sealed record WebScheduleSnapshot(
  int LightMode,
  int DayStartHour,
  int DayStartMin,
  int DayEndHour,
  int DayEndMin,
  int AirMode,
  int AirStartHour,
  int AirStartMin,
  int AirEndHour,
  int AirEndMin,
  int FilterMode,
  int FilterStartHour,
  int FilterStartMin,
  int FilterEndHour,
  int FilterEndMin,
  int HeaterMode,
  int ServoPreOffMins);

public sealed record WebFeedingSnapshot(int Hour, int Minute, int Freq, long LastFeedEpoch, bool Active);

public sealed record WebNetworkSnapshot(
  string Ip,
  bool ApMode,
  string Ssid,
  int Clients,
  bool StaConnected,
  bool StaConnecting,
  bool ServiceMode,
  string StaSsid,
  string ConfiguredStaSsid,
  string ConfiguredApSsid,
  long StaLastConnectedEpoch,
  bool TimeSyncInProgress,
  long LastTimeSyncEpoch,
  bool LastTimeSyncOk,
  string LastTimeSyncStatus,
  bool BleAdvertising,
  bool BleConnected,
  bool BleActive);

public sealed record WebClockSnapshot(int Hour, int Minute, int Second, int Day, int Month, int Year);

public sealed record WebFirmwareSnapshot(
  string Name,
  string Version,
  string BuildRef,
  string BuildDate,
  string BuildTime,
  string IdfVersion);

public sealed record WebStatusResponse(
  string SchemaVersion,
  WebTemperatureSnapshot Temperature,
  WebBatterySnapshot Battery,
  WebRelaySnapshot Relays,
  WebScheduleSnapshot Schedule,
  WebFeedingSnapshot Feeding,
  WebNetworkSnapshot Network,
  WebClockSnapshot Clock,
  WebFirmwareSnapshot Firmware);

public sealed record WebLogsResponse(IReadOnlyList<string> Normal, IReadOnlyList<string> Critical);

public sealed record WebActionResponse(
  bool Success,
  string Code,
  string? Message = null,
  string SchemaVersion = WebApiContract.SchemaVersion);
