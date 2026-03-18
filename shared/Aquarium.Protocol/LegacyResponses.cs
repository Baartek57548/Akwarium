using System.Text.Json.Serialization;

namespace Aquarium.Protocol;

public sealed class AquariumCommandResult
{
  public static AquariumCommandResult Empty { get; } = new()
  {
    Type = "info",
    Code = "no_response"
  };

  [JsonPropertyName("t")]
  public string Type { get; init; } = string.Empty;

  [JsonPropertyName("c")]
  public string Code { get; init; } = string.Empty;

  [JsonIgnore]
  public bool IsSuccess => string.Equals(Type, "ack", StringComparison.OrdinalIgnoreCase);
}

public sealed class AquariumOtaState
{
  public static AquariumOtaState Empty { get; } = new();

  [JsonPropertyName("t")]
  public string Type { get; init; } = "info";

  [JsonPropertyName("c")]
  public string Code { get; init; } = "ready";

  [JsonPropertyName("busy")]
  public bool IsBusy { get; init; }

  [JsonPropertyName("ota")]
  public string Transport { get; init; } = "idle";

  [JsonPropertyName("rx")]
  public int ReceivedBytes { get; init; }

  [JsonPropertyName("size")]
  public int TotalBytes { get; init; }

  [JsonPropertyName("chunk")]
  public int RecommendedChunkSizeBytes { get; init; }

  [JsonPropertyName("reboot")]
  public int RebootDelayMilliseconds { get; init; }

  [JsonPropertyName("ver")]
  public string DeclaredVersion { get; init; } = string.Empty;

  [JsonPropertyName("prj")]
  public string DeclaredProject { get; init; } = string.Empty;

  [JsonIgnore]
  public bool IsSuccess => string.Equals(Type, "ack", StringComparison.OrdinalIgnoreCase);
}

public sealed class AquariumOtaControlRequest
{
  public AquariumOtaControlRequest(string action, int? size = null, string? version = null, string? project = null)
  {
    Action = action;
    Size = size;
    Version = version;
    Project = project;
  }

  [JsonPropertyName("action")]
  public string Action { get; }

  [JsonPropertyName("size")]
  [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
  public int? Size { get; }

  [JsonPropertyName("version")]
  [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
  public string? Version { get; }

  [JsonPropertyName("project")]
  [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
  public string? Project { get; }
}

public sealed record OtaUploadProgress(int BytesSent, int TotalBytes, string Stage)
{
  public double Progress => TotalBytes <= 0 ? 0d : Math.Clamp(BytesSent / (double)TotalBytes, 0d, 1d);
}

public sealed record OtaUploadResult(bool IsSuccess, string Code, string Message, int RebootDelayMilliseconds);
