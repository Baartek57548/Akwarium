using System.Net;
using System.Text;
using Aquarium.EmulatorCore;
using Aquarium.Protocol;

namespace Aquarium.Emulator;

internal sealed class EmulatorHttpApiServer : IDisposable
{
  private readonly EmulatedDeviceCore _core;
  private readonly HttpListener _listener = new();
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
        await SendJsonAsync(context.Response, FirmwareJsonWriter.BuildStatusJson(_core.Snapshot())).ConfigureAwait(false);
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
        await ExecuteActionAsync(action, fields).ConfigureAwait(false);
        await SendTextAsync(context.Response, "OK").ConfigureAwait(false);
        return;
      }

      await SendTextAsync(context.Response, "Not found", HttpStatusCode.NotFound).ConfigureAwait(false);
    }
    catch (Exception ex)
    {
      await SendTextAsync(context.Response, ex.Message, HttpStatusCode.InternalServerError).ConfigureAwait(false);
    }
  }

  private async Task ExecuteActionAsync(string action, Dictionary<string, string> fields)
  {
    switch (action)
    {
      case "start_device":
        _core.Start();
        break;
      case "stop_device":
        _core.Stop();
        break;
      case "simulate_sensor_error":
        _core.SimulateSensorError(fields.TryGetValue("enabled", out var sensorValue) && ParseBool(sensorValue));
        break;
      case "simulate_temperature_change":
        if (fields.TryGetValue("temperature", out var temperatureValue) && double.TryParse(temperatureValue, out var temperature))
        {
          _core.SetTemperature(temperature);
        }
        break;
      case "simulate_feeder_jam":
        _core.SimulateFeederJam(fields.TryGetValue("enabled", out var jamValue) && ParseBool(jamValue));
        break;
      case "feed_now":
        await _core.SendCommandAsync(new DeviceCommand("feed_now")).ConfigureAwait(false);
        break;
      case "set_servo":
        if (fields.TryGetValue("angle", out var angleValue) && int.TryParse(angleValue, out var angle))
        {
          await _core.SendCommandAsync(new DeviceCommand("set_servo", angle)).ConfigureAwait(false);
        }
        break;
      case "clear_servo":
        await _core.SendCommandAsync(new DeviceCommand("clear_servo")).ConfigureAwait(false);
        break;
      case "clear_critical_logs":
        await _core.SendCommandAsync(new DeviceCommand("clear_critical_logs")).ConfigureAwait(false);
        break;
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

  private static bool ParseBool(string value)
  {
    return value.Equals("1", StringComparison.OrdinalIgnoreCase) ||
           value.Equals("true", StringComparison.OrdinalIgnoreCase) ||
           value.Equals("yes", StringComparison.OrdinalIgnoreCase);
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
