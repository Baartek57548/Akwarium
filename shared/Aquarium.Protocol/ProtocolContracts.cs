using System.Text.Json;
using System.Text.Json.Serialization;
using Aquarium.Models;

namespace Aquarium.Protocol;

public static class ProtocolMessageTypes
{
  public const string Command = "command";
  public const string Status = "status";
  public const string Event = "event";
  public const string Response = "response";
}

public sealed record ProtocolEnvelope<TPayload>(
  [property: JsonPropertyName("type")] string Type,
  [property: JsonPropertyName("name")] string Name,
  [property: JsonPropertyName("payload")] TPayload Payload);

public sealed record DeviceCommand(
  [property: JsonPropertyName("action")] string Action,
  [property: JsonPropertyName("angle")] int? Angle = null,
  [property: JsonPropertyName("data")] JsonElement? Data = null);

public sealed record DeviceResponse(
  [property: JsonPropertyName("success")] bool Success,
  [property: JsonPropertyName("code")] string Code,
  [property: JsonPropertyName("message")] string? Message = null,
  [property: JsonPropertyName("payload")] JsonElement? Payload = null);

public sealed record DeviceEvent(
  [property: JsonPropertyName("level")] string Level,
  [property: JsonPropertyName("code")] string Code,
  [property: JsonPropertyName("message")] string? Message = null,
  [property: JsonPropertyName("payload")] JsonElement? Payload = null);

public sealed record OtaRequest(
  [property: JsonPropertyName("action")] string Action,
  [property: JsonPropertyName("size")] int? Size = null,
  [property: JsonPropertyName("version")] string? Version = null,
  [property: JsonPropertyName("project")] string? Project = null);

public interface IDeviceProtocol
{
  string Serialize<TPayload>(ProtocolEnvelope<TPayload> message);

  bool TryDeserialize<TPayload>(string json, out ProtocolEnvelope<TPayload>? message);

  ProtocolEnvelope<DeviceCommand> CreateCommand(DeviceCommand command);

  ProtocolEnvelope<DeviceResponse> CreateResponse(DeviceResponse response);

  ProtocolEnvelope<DeviceEvent> CreateEvent(DeviceEvent eventMessage);

  ProtocolEnvelope<OtaRequest> CreateOtaRequest(OtaRequest request);
}

public interface IDeviceConnection
{
  bool IsConnected { get; }

  Task ConnectAsync(CancellationToken cancellationToken = default);

  Task DisconnectAsync(CancellationToken cancellationToken = default);
}

public interface IDeviceController
{
  Task<DeviceStatus> ReadStatusAsync(CancellationToken cancellationToken = default);

  Task<DeviceConfig> ReadSettingsAsync(CancellationToken cancellationToken = default);

  Task<SystemInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default);

  Task<DeviceResponse> SendCommandAsync(DeviceCommand command, CancellationToken cancellationToken = default);

  Task<DeviceResponse> SaveSettingsAsync(DeviceConfig settings, CancellationToken cancellationToken = default);

  Task<DeviceResponse> StartOtaAsync(OtaRequest request, CancellationToken cancellationToken = default);

  Task<DeviceResponse> FinishOtaAsync(CancellationToken cancellationToken = default);

  Task<DeviceResponse> AbortOtaAsync(CancellationToken cancellationToken = default);
}

public sealed class JsonDeviceProtocol : IDeviceProtocol
{
  private static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web)
  {
    DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
  };

  public string Serialize<TPayload>(ProtocolEnvelope<TPayload> message)
  {
    return JsonSerializer.Serialize(message, Options);
  }

  public bool TryDeserialize<TPayload>(string json, out ProtocolEnvelope<TPayload>? message)
  {
    try
    {
      message = JsonSerializer.Deserialize<ProtocolEnvelope<TPayload>>(json, Options);
      return message is not null;
    }
    catch
    {
      message = default;
      return false;
    }
  }

  public ProtocolEnvelope<DeviceCommand> CreateCommand(DeviceCommand command)
  {
    return new ProtocolEnvelope<DeviceCommand>(ProtocolMessageTypes.Command, command.Action, command);
  }

  public ProtocolEnvelope<DeviceResponse> CreateResponse(DeviceResponse response)
  {
    return new ProtocolEnvelope<DeviceResponse>(ProtocolMessageTypes.Response, response.Code, response);
  }

  public ProtocolEnvelope<DeviceEvent> CreateEvent(DeviceEvent eventMessage)
  {
    return new ProtocolEnvelope<DeviceEvent>(ProtocolMessageTypes.Event, eventMessage.Code, eventMessage);
  }

  public ProtocolEnvelope<OtaRequest> CreateOtaRequest(OtaRequest request)
  {
    return new ProtocolEnvelope<OtaRequest>(ProtocolMessageTypes.Command, request.Action, request);
  }
}
