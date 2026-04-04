using System.Net;
using System.Globalization;
using System.Text;
using System.Threading.Channels;
using Aquarium.EmulatorCore;
using Aquarium.Models;
using Aquarium.Protocol;

namespace Aquarium.Emulator;

internal sealed class EmulatorHttpApiServer : IDisposable
{
  private readonly EmulatedDeviceCore _core;
  private readonly HttpListener _listener = new();
  private string _configuredStaSsid = "Aquarium-STA";
  private string _configuredApSsid = "Aquarium-AP";
  private CancellationTokenSource? _cts;
  private Task? _loopTask;

  public EmulatorHttpApiServer(EmulatedDeviceCore core, int port = 5080)
  {
    _core = core;
    Port = port;
    _listener.Prefixes.Add($"http://127.0.0.1:{Port}/");
  }

  public int Port { get; }

  public bool IsRunning => _listener.IsListening;

  public Task StartAsync()
  {
    if (_listener.IsListening)
    {
      return Task.CompletedTask;
    }

    _listener.Start();
    _cts = new CancellationTokenSource();
    _loopTask = Task.Run(() => RunLoopAsync(_cts.Token));
    return Task.CompletedTask;
  }

  public async Task StopAsync()
  {
    if (!_listener.IsListening)
    {
      return;
    }

    _cts?.Cancel();

    try
    {
      if (_loopTask is not null)
      {
        await _loopTask.ConfigureAwait(false);
      }
    }
    catch (OperationCanceledException)
    {
    }
    finally
    {
      _listener.Stop();
      _cts?.Dispose();
      _cts = null;
      _loopTask = null;
    }
  }

  public void Dispose()
  {
    _listener.Close();
    _cts?.Dispose();
  }

  private async Task RunLoopAsync(CancellationToken cancellationToken)
  {
    while (!cancellationToken.IsCancellationRequested)
    {
      HttpListenerContext? context = null;
      try
      {
        context = await _listener.GetContextAsync().WaitAsync(cancellationToken).ConfigureAwait(false);
      }
      catch (OperationCanceledException)
      {
        break;
      }
      catch (HttpListenerException)
      {
        break;
      }

      if (context is not null)
      {
        _ = Task.Run(() => HandleRequestAsync(context, cancellationToken));
      }
    }
  }

  private async Task HandleRequestAsync(HttpListenerContext context, CancellationToken cancellationToken)
  {
    try
    {
      var request = context.Request;
      var path = request.Url?.AbsolutePath ?? "/";

      if (request.HttpMethod == "GET" && path.Equals("/api/status", StringComparison.OrdinalIgnoreCase))
      {
        await SendJsonAsync(
          context.Response,
          FirmwareJsonWriter.BuildStatusJson(_core.Snapshot(), _configuredStaSsid, _configuredApSsid)).ConfigureAwait(false);
        return;
      }

      if (request.HttpMethod == "GET" && path.Equals("/api/logs", StringComparison.OrdinalIgnoreCase))
      {
        await SendJsonAsync(context.Response, FirmwareJsonWriter.BuildLogsJson(_core.Logs)).ConfigureAwait(false);
        return;
      }

      if (request.HttpMethod == "GET" && path.Equals("/api/events", StringComparison.OrdinalIgnoreCase))
      {
        await HandleEventStreamAsync(context, cancellationToken).ConfigureAwait(false);
        return;
      }

      if (request.HttpMethod == "GET" && path.Equals("/api/settings", StringComparison.OrdinalIgnoreCase))
      {
        await SendJsonAsync(context.Response, FirmwareJsonWriter.BuildSettingsJson(_core.Snapshot().Config)).ConfigureAwait(false);
        return;
      }

      if (request.HttpMethod == "GET" && path.Equals("/api/device", StringComparison.OrdinalIgnoreCase))
      {
        await SendJsonAsync(context.Response, FirmwareJsonWriter.BuildDeviceInfoJson(_core.SnapshotSystemInfo())).ConfigureAwait(false);
        return;
      }

      if (request.HttpMethod == "POST" && path.Equals("/api/action", StringComparison.OrdinalIgnoreCase))
      {
        var body = await ReadBodyAsync(request, cancellationToken).ConfigureAwait(false);
        var fields = ParseFormData(body);
        var action = fields.TryGetValue("action", out var actionValue) ? actionValue : string.Empty;
        var response = await ExecuteActionAsync(action, fields, cancellationToken).ConfigureAwait(false);
        await SendJsonAsync(
          context.Response,
          FirmwareJsonWriter.BuildActionResponseJson(response),
          response.Success ? HttpStatusCode.OK : HttpStatusCode.BadRequest).ConfigureAwait(false);
        return;
      }

      await SendTextAsync(context.Response, "Not found", HttpStatusCode.NotFound).ConfigureAwait(false);
    }
    catch (Exception ex)
    {
      await SendTextAsync(context.Response, ex.Message, HttpStatusCode.InternalServerError).ConfigureAwait(false);
    }
  }

  private async Task<DeviceResponse> ExecuteActionAsync(
    string action,
    Dictionary<string, string> fields,
    CancellationToken cancellationToken)
  {
    switch (action)
    {
      case "start_device":
        _core.Start();
        return new DeviceResponse(true, "start_device", "Emulator device started.");
      case "stop_device":
        _core.Stop();
        return new DeviceResponse(true, "stop_device", "Emulator device stopped.");
      case "simulate_sensor_error":
        _core.SimulateSensorError(fields.TryGetValue("enabled", out var sensorValue) &&
                                  TryParseBool(sensorValue, out var sensorEnabled) &&
                                  sensorEnabled);
        return new DeviceResponse(true, "simulate_sensor_error");
      case "simulate_temperature_change":
        if (fields.TryGetValue("temperature", out var temperatureValue) &&
            double.TryParse(temperatureValue, NumberStyles.Float, CultureInfo.InvariantCulture, out var temperature))
        {
          _core.SetTemperature(temperature);
          return new DeviceResponse(true, "simulate_temperature_change");
        }
        return new DeviceResponse(false, "invalid_temperature", "Temperature payload is invalid.");
      case "simulate_feeder_jam":
        _core.SimulateFeederJam(fields.TryGetValue("enabled", out var jamValue) &&
                                TryParseBool(jamValue, out var jamEnabled) &&
                                jamEnabled);
        return new DeviceResponse(true, "simulate_feeder_jam");
      case "feed_now":
        return await _core.SendCommandAsync(new DeviceCommand("feed_now"), cancellationToken).ConfigureAwait(false);
      case "set_light":
      case "set_filter":
        if (!fields.TryGetValue("state", out var stateValue))
        {
          return new DeviceResponse(false, "missing_state", "Missing state parameter.");
        }

        if (!TryParseBool(stateValue, out var desiredState))
        {
          return new DeviceResponse(false, "invalid_state", "State parameter is invalid.");
        }

        var currentConfig = await _core.ReadSettingsAsync(cancellationToken).ConfigureAwait(false);
        var desiredMode = desiredState ? ScheduleMode.AlwaysOn : ScheduleMode.AlwaysOff;
        var updatedConfig = action == "set_light"
          ? currentConfig with { Lighting = currentConfig.Lighting with { Mode = desiredMode } }
          : currentConfig with { Filter = currentConfig.Filter with { Mode = desiredMode } };
        return await _core.SaveSettingsAsync(updatedConfig, cancellationToken).ConfigureAwait(false);
      case "set_servo":
        if (fields.TryGetValue("angle", out var angleValue) && int.TryParse(angleValue, out var angle))
        {
          return await _core.SendCommandAsync(new DeviceCommand("set_servo", angle), cancellationToken).ConfigureAwait(false);
        }
        return new DeviceResponse(false, "invalid_angle", "Angle parameter is invalid.");
      case "clear_servo":
        return await _core.SendCommandAsync(new DeviceCommand("clear_servo"), cancellationToken).ConfigureAwait(false);
      case "clear_critical_logs":
        return await _core.SendCommandAsync(new DeviceCommand("clear_critical_logs"), cancellationToken).ConfigureAwait(false);
      case "save_schedule":
      {
        var config = await _core.ReadSettingsAsync(cancellationToken).ConfigureAwait(false);
        var updated = TryApplySchedulePatch(config, fields, out var code, out var message);
        if (updated is null)
        {
          return new DeviceResponse(false, code, message);
        }

        var result = await _core.SaveSettingsAsync(updated, cancellationToken).ConfigureAwait(false);
        return result with { Code = "settings_saved" };
      }
      case "save_network":
      {
        var anyField = false;
        if (fields.TryGetValue("staSsid", out var staSsid))
        {
          _configuredStaSsid = staSsid.Trim();
          anyField = true;
        }
        if (fields.TryGetValue("apSsid", out var apSsid))
        {
          _configuredApSsid = apSsid.Trim();
          anyField = true;
        }

        if (!anyField)
        {
          return new DeviceResponse(false, "empty_settings", "No network fields were provided.");
        }

        return new DeviceResponse(true, "settings_saved", "Network settings stored in emulator memory.");
      }
      case "restart_device":
        _core.Stop();
        _core.Start();
        return new DeviceResponse(true, "restart_device", "Emulator device restarted.");
      case "factory_reset":
      {
        var defaultConfig = new EmulatedDeviceCore();
        var settings = await defaultConfig.ReadSettingsAsync(cancellationToken).ConfigureAwait(false);
        await _core.SaveSettingsAsync(settings, cancellationToken).ConfigureAwait(false);
        return new DeviceResponse(true, "factory_reset", "Emulator settings restored to defaults.");
      }
      default:
        return new DeviceResponse(false, "unknown_action", $"Unknown action: {action}");
    }
  }

  private static async Task<string> ReadBodyAsync(HttpListenerRequest request, CancellationToken cancellationToken)
  {
    using var reader = new StreamReader(request.InputStream, request.ContentEncoding ?? Encoding.UTF8, detectEncodingFromByteOrderMarks: true, leaveOpen: true);
    return await reader.ReadToEndAsync().WaitAsync(cancellationToken).ConfigureAwait(false);
  }

  private static Dictionary<string, string> ParseFormData(string body)
  {
    var result = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
    if (string.IsNullOrWhiteSpace(body))
    {
      return result;
    }

    foreach (var pair in body.Split('&', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
    {
      var index = pair.IndexOf('=');
      if (index <= 0)
      {
        continue;
      }

      var key = WebUtility.UrlDecode(pair[..index]) ?? string.Empty;
      var value = WebUtility.UrlDecode(pair[(index + 1)..]) ?? string.Empty;
      if (!string.IsNullOrWhiteSpace(key))
      {
        result[key] = value;
      }
    }

    return result;
  }

  private static bool TryParseBool(string value, out bool result)
  {
    if (value.Equals("1", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("true", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("yes", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("on", StringComparison.OrdinalIgnoreCase))
    {
      result = true;
      return true;
    }

    if (value.Equals("0", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("false", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("no", StringComparison.OrdinalIgnoreCase) ||
        value.Equals("off", StringComparison.OrdinalIgnoreCase))
    {
      result = false;
      return true;
    }

    result = false;
    return false;
  }

  private static bool TryParseTime(string rawValue, out int hour, out int minute)
  {
    hour = 0;
    minute = 0;
    var parts = rawValue.Split(':', StringSplitOptions.TrimEntries);
    if (parts.Length != 2 ||
        !int.TryParse(parts[0], out hour) ||
        !int.TryParse(parts[1], out minute))
    {
      return false;
    }

    if (hour is < 0 or > 23 || minute is < 0 or > 59)
    {
      return false;
    }

    return true;
  }

  private static DeviceConfig? TryApplySchedulePatch(
    DeviceConfig current,
    Dictionary<string, string> fields,
    out string code,
    out string message)
  {
    code = "settings_saved";
    message = "Settings stored in emulator memory.";

    var updated = current;
    var lighting = current.Lighting;
    var aeration = current.Aeration;
    var filter = current.Filter;
    var hasAnyField = false;

    if (fields.TryGetValue("lightMode", out var lightModeRaw))
    {
      if (!int.TryParse(lightModeRaw, out var lightMode))
      {
        code = "invalid_payload";
        message = "lightMode must be a valid integer.";
        return null;
      }

      lighting = lighting with { Mode = (ScheduleMode)Math.Clamp(lightMode, 0, 2) };
      hasAnyField = true;
    }

    if (fields.TryGetValue("dayStart", out var dayStartRaw))
    {
      if (!TryParseTime(dayStartRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "dayStart must be a valid HH:MM value.";
        return null;
      }

      lighting = lighting with { StartHour = hour, StartMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("dayEnd", out var dayEndRaw))
    {
      if (!TryParseTime(dayEndRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "dayEnd must be a valid HH:MM value.";
        return null;
      }

      lighting = lighting with { EndHour = hour, EndMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("aerationMode", out var aerationModeRaw))
    {
      if (!int.TryParse(aerationModeRaw, out var aerationMode))
      {
        code = "invalid_payload";
        message = "aerationMode must be a valid integer.";
        return null;
      }

      aeration = aeration with { Mode = (ScheduleMode)Math.Clamp(aerationMode, 0, 2) };
      hasAnyField = true;
    }

    if (fields.TryGetValue("airOn", out var airOnRaw))
    {
      if (!TryParseTime(airOnRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "airOn must be a valid HH:MM value.";
        return null;
      }

      aeration = aeration with { StartHour = hour, StartMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("airOff", out var airOffRaw))
    {
      if (!TryParseTime(airOffRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "airOff must be a valid HH:MM value.";
        return null;
      }

      aeration = aeration with { EndHour = hour, EndMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("filterMode", out var filterModeRaw))
    {
      if (!int.TryParse(filterModeRaw, out var filterMode))
      {
        code = "invalid_payload";
        message = "filterMode must be a valid integer.";
        return null;
      }

      filter = filter with { Mode = (ScheduleMode)Math.Clamp(filterMode, 0, 2) };
      hasAnyField = true;
    }

    if (fields.TryGetValue("filterOn", out var filterOnRaw))
    {
      if (!TryParseTime(filterOnRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "filterOn must be a valid HH:MM value.";
        return null;
      }

      filter = filter with { StartHour = hour, StartMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("filterOff", out var filterOffRaw))
    {
      if (!TryParseTime(filterOffRaw, out var hour, out var minute))
      {
        code = "invalid_payload";
        message = "filterOff must be a valid HH:MM value.";
        return null;
      }

      filter = filter with { EndHour = hour, EndMinute = minute };
      hasAnyField = true;
    }

    if (fields.TryGetValue("feedFreq", out var feedFreqRaw))
    {
      if (!int.TryParse(feedFreqRaw, out var feedFreq))
      {
        code = "invalid_payload";
        message = "feedFreq must be a valid integer.";
        return null;
      }

      updated = updated with { FeedMode = Math.Clamp(feedFreq, 0, 3) };
      hasAnyField = true;
    }

    if (fields.TryGetValue("feedTime", out var feedTimeRaw))
    {
      if (!TryParseTime(feedTimeRaw, out var feedHour, out var feedMinute))
      {
        code = "invalid_payload";
        message = "feedTime must be a valid HH:MM value.";
        return null;
      }

      updated = updated with { FeedHour = feedHour, FeedMinute = feedMinute };
      hasAnyField = true;
    }

    if (!hasAnyField)
    {
      code = "empty_settings";
      message = "No schedule fields were provided.";
      return null;
    }

    return updated with
    {
      Lighting = lighting,
      Aeration = aeration,
      Filter = filter
    };
  }

  private async Task HandleEventStreamAsync(
    HttpListenerContext context,
    CancellationToken cancellationToken)
  {
    var response = context.Response;
    response.StatusCode = (int)HttpStatusCode.OK;
    response.ContentType = "text/event-stream; charset=utf-8";
    response.SendChunked = true;
    response.KeepAlive = true;
    response.Headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
    response.Headers["X-Accel-Buffering"] = "no";

    using var writer = new StreamWriter(response.OutputStream, new UTF8Encoding(false), leaveOpen: true)
    {
      AutoFlush = true,
      NewLine = "\n"
    };

    var signal = Channel.CreateUnbounded<bool>();
    void OnStateChanged(object? _, EventArgs __) => signal.Writer.TryWrite(true);

    _core.StateChanged += OnStateChanged;

    try
    {
      await WriteSseEventAsync(
        writer,
        "ready",
        FirmwareJsonWriter.BuildActionResponseJson(new DeviceResponse(true, "sse_ready", "Event stream connected.")),
        cancellationToken).ConfigureAwait(false);
      await WriteSseSnapshotAsync(writer, cancellationToken).ConfigureAwait(false);

      while (!cancellationToken.IsCancellationRequested)
      {
        var signalTask = signal.Reader.WaitToReadAsync(cancellationToken).AsTask();
        var heartbeatTask = Task.Delay(TimeSpan.FromSeconds(15), cancellationToken);
        var completed = await Task.WhenAny(signalTask, heartbeatTask).ConfigureAwait(false);

        if (completed == signalTask)
        {
          if (!await signalTask.ConfigureAwait(false))
          {
            break;
          }

          while (signal.Reader.TryRead(out _))
          {
          }

          await WriteSseSnapshotAsync(writer, cancellationToken).ConfigureAwait(false);
          continue;
        }

        await writer.WriteAsync(": keep-alive\n\n").ConfigureAwait(false);
        await writer.FlushAsync().ConfigureAwait(false);
      }
    }
    catch (OperationCanceledException)
    {
    }
    catch (ObjectDisposedException)
    {
    }
    catch (IOException)
    {
    }
    catch (HttpListenerException)
    {
    }
    finally
    {
      _core.StateChanged -= OnStateChanged;
      response.Close();
    }
  }

  private async Task WriteSseSnapshotAsync(StreamWriter writer, CancellationToken cancellationToken)
  {
    await WriteSseEventAsync(
      writer,
      "status",
      FirmwareJsonWriter.BuildStatusJson(_core.Snapshot(), _configuredStaSsid, _configuredApSsid),
      cancellationToken).ConfigureAwait(false);
    await WriteSseEventAsync(
      writer,
      "logs",
      FirmwareJsonWriter.BuildLogsJson(_core.Logs),
      cancellationToken).ConfigureAwait(false);
  }

  private static async Task WriteSseEventAsync(
    StreamWriter writer,
    string eventName,
    string payload,
    CancellationToken cancellationToken)
  {
    await writer.WriteAsync($"event: {eventName}\n".AsMemory(), cancellationToken).ConfigureAwait(false);
    await writer.WriteAsync($"data: {payload}\n\n".AsMemory(), cancellationToken).ConfigureAwait(false);
    await writer.FlushAsync().ConfigureAwait(false);
  }

  private static async Task SendJsonAsync(HttpListenerResponse response, string json, HttpStatusCode status = HttpStatusCode.OK)
  {
    response.StatusCode = (int)status;
    response.ContentType = "application/json; charset=utf-8";
    var buffer = Encoding.UTF8.GetBytes(json);
    response.ContentLength64 = buffer.Length;
    await response.OutputStream.WriteAsync(buffer, 0, buffer.Length).ConfigureAwait(false);
    response.Close();
  }

  private static async Task SendTextAsync(HttpListenerResponse response, string text, HttpStatusCode status = HttpStatusCode.OK)
  {
    response.StatusCode = (int)status;
    response.ContentType = "text/plain; charset=utf-8";
    var buffer = Encoding.UTF8.GetBytes(text);
    response.ContentLength64 = buffer.Length;
    await response.OutputStream.WriteAsync(buffer, 0, buffer.Length).ConfigureAwait(false);
    response.Close();
  }
}
