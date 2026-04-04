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

  public static string BuildStatusJson(
    DeviceStatus status,
    string configuredStaSsid = "",
    string configuredApSsid = "")
  {
    var payload = status.ToWebStatusResponse();
    if (!string.IsNullOrWhiteSpace(configuredStaSsid) || !string.IsNullOrWhiteSpace(configuredApSsid))
    {
      payload = payload with
      {
        Network = payload.Network with
        {
          ConfiguredStaSsid = configuredStaSsid,
          ConfiguredApSsid = configuredApSsid,
          StaSsid = string.IsNullOrWhiteSpace(configuredStaSsid) ? payload.Network.StaSsid : configuredStaSsid
        }
      };
    }

    return JsonSerializer.Serialize(payload, Options);
  }

  public static string BuildLogsJson(IEnumerable<string> logs)
  {
    return JsonSerializer.Serialize(logs.ToWebLogsResponse(), Options);
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

  public static string BuildActionResponseJson(DeviceResponse response)
  {
    return JsonSerializer.Serialize(response.ToWebActionResponse(), Options);
  }
}
