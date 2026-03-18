using System.Buffers.Binary;
using System.Text;
using System.Text.Json;
using Aquarium.EmulatorCore;
using Microsoft.Extensions.Logging;

namespace AquariumController.Mobile.Services;

public sealed class EmulatorBluetoothService : IBluetoothService, IDisposable
{
	private static readonly Guid FakeDeviceId = Guid.Parse("00000000-0000-0000-0000-000000000001");
	private static readonly JsonSerializerOptions SerializerOptions = new(JsonSerializerDefaults.Web)
	{
		DefaultIgnoreCondition = System.Text.Json.Serialization.JsonIgnoreCondition.WhenWritingNull
	};

	private readonly ILogger<EmulatorBluetoothService> _logger;
	private readonly object _gate = new();
	private readonly EmulatedDeviceCore _core = new();
	private bool _disposed;
	private bool _connected;

	public EmulatorBluetoothService(ILogger<EmulatorBluetoothService> logger)
	{
		_logger = logger;
	}

	public event EventHandler<BluetoothAdapterStateChangedEventArgs>? AdapterStateChanged;

	public event EventHandler<BluetoothConnectionChangedEventArgs>? ConnectionChanged;

	public BleAdapterStatus AdapterState => BleAdapterStatus.On;

	public bool IsScanning => false;

	public bool IsConnected => _connected;

	public BleDeviceInfo? ConnectedDevice => _connected ? new BleDeviceInfo(FakeDeviceId, "Aquarium Emulator", -42) : null;

	public Task<bool> EnsurePermissionsAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();
		return Task.FromResult(true);
	}

	public Task<IReadOnlyList<BleDeviceInfo>> ScanForDevicesAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();
		AdapterStateChanged?.Invoke(this, new BluetoothAdapterStateChangedEventArgs(BleAdapterStatus.Unknown, BleAdapterStatus.On));
		return Task.FromResult<IReadOnlyList<BleDeviceInfo>>([new BleDeviceInfo(FakeDeviceId, "Aquarium Emulator", -42)]);
	}

	public Task StopScanningAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();
		return Task.CompletedTask;
	}

	public async Task ConnectAsync(Guid deviceId, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		if (deviceId != FakeDeviceId)
		{
			throw new InvalidOperationException("Nieznany emulator BLE.");
		}

		lock (_gate)
		{
			_connected = true;
		}

		await _core.ConnectAsync(cancellationToken).ConfigureAwait(false);
		ConnectionChanged?.Invoke(this, new BluetoothConnectionChangedEventArgs(
			BluetoothConnectionState.Connected,
			ConnectedDevice,
			"Połączono z emulatorem urządzenia."));
	}

	public async Task DisconnectAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		lock (_gate)
		{
			_connected = false;
		}

		await _core.DisconnectAsync(cancellationToken).ConfigureAwait(false);
		ConnectionChanged?.Invoke(this, new BluetoothConnectionChangedEventArgs(
			BluetoothConnectionState.Disconnected,
			null,
			"Rozłączono emulowany sterownik."));
	}

	public Task<byte[]> ReadCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		if (serviceUuid != AquariumBleContract.ServiceUuid)
		{
			throw new InvalidOperationException("Nieznany serwis BLE emulatora.");
		}

		var snapshot = _core.Snapshot();
		var info = _core.SnapshotSystemInfo();
		string payload = characteristicUuid switch
		{
			_ when characteristicUuid == AquariumBleContract.StatusCharacteristicUuid => JsonSerializer.Serialize(snapshot.ToLegacyStatus(), SerializerOptions),
			_ when characteristicUuid == AquariumBleContract.SettingsCharacteristicUuid => JsonSerializer.Serialize(snapshot.Config.ToLegacySettings(), SerializerOptions),
			_ when characteristicUuid == AquariumBleContract.DeviceInfoCharacteristicUuid => JsonSerializer.Serialize(info.ToLegacyDeviceInfo(), SerializerOptions),
			_ when characteristicUuid == AquariumBleContract.ResultCharacteristicUuid => JsonSerializer.Serialize(new AquariumCommandResult { Type = "info", Code = "ready" }, SerializerOptions),
			_ when characteristicUuid == AquariumBleContract.OtaControlCharacteristicUuid => JsonSerializer.Serialize(new AquariumOtaState { Type = "info", Code = info.IsOtaInProgress ? "ota_receiving" : "ready" }, SerializerOptions),
			_ => throw new InvalidOperationException("Nieznana charakterystyka emulatora BLE.")
		};

		return Task.FromResult(Encoding.UTF8.GetBytes(payload));
	}

	public async Task WriteCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, byte[] data, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		if (serviceUuid != AquariumBleContract.ServiceUuid)
		{
			throw new InvalidOperationException("Nieznany serwis BLE emulatora.");
		}

		if (characteristicUuid == AquariumBleContract.CommandCharacteristicUuid)
		{
			var commandPayload = JsonSerializer.Deserialize<AquariumCommandPayload>(data, SerializerOptions);
			if (commandPayload is not null)
			{
				var command = commandPayload.Action switch
				{
					"feed_now" => AquariumCommand.FeedNow(),
					"set_servo" => AquariumCommand.SetServo(commandPayload.Angle ?? 0),
					"clear_servo" => AquariumCommand.ClearServo(),
					"clear_critical_logs" => AquariumCommand.ClearCriticalLogs(),
					_ => null
				};

				if (command is not null)
				{
					await SendCommandAsync(command, cancellationToken).ConfigureAwait(false);
				}
			}
			return;
		}

		if (characteristicUuid == AquariumBleContract.SettingsCharacteristicUuid)
		{
			var settings = JsonSerializer.Deserialize<AquariumSettings>(data, SerializerOptions);
			if (settings is not null)
			{
				await SaveSettingsAsync(settings, cancellationToken).ConfigureAwait(false);
			}
			return;
		}

		if (characteristicUuid == AquariumBleContract.OtaControlCharacteristicUuid)
		{
			var request = JsonSerializer.Deserialize<AquariumOtaControlRequest>(data, SerializerOptions);
			if (request is null)
			{
				return;
			}

			switch (request.Action)
			{
				case "begin":
					await _core.StartOtaAsync(new OtaRequest("ble", request.Size, request.Version, request.Project), cancellationToken).ConfigureAwait(false);
					break;
				case "finish":
					await _core.FinishOtaAsync(cancellationToken).ConfigureAwait(false);
					break;
				case "abort":
					await _core.AbortOtaAsync(cancellationToken).ConfigureAwait(false);
					break;
			}
		}
	}

	public Task<AquariumStatus> ReadStatusAsync(CancellationToken cancellationToken = default)
	{
		return ReadStatusCoreAsync(cancellationToken);
	}

	public Task<AquariumSettings> ReadSettingsAsync(CancellationToken cancellationToken = default)
	{
		return ReadSettingsCoreAsync(cancellationToken);
	}

	public Task<AquariumDeviceInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default)
	{
		return ReadDeviceInfoCoreAsync(cancellationToken);
	}

	public async Task<AquariumCommandResult> SendCommandAsync(AquariumCommand command, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var response = await _core.SendCommandAsync(new DeviceCommand(command.Action, command.Angle), cancellationToken).ConfigureAwait(false);
		return new AquariumCommandResult
		{
			Type = response.Success ? "ack" : "err",
			Code = response.Code
		};
	}

	public async Task<AquariumCommandResult> SaveSettingsAsync(AquariumSettings settings, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var response = await _core.SaveSettingsAsync(settings.ToDeviceConfig(), cancellationToken).ConfigureAwait(false);
		return new AquariumCommandResult
		{
			Type = response.Success ? "ack" : "err",
			Code = response.Code
		};
	}

	public async Task<OtaUploadResult> UploadFirmwareAsync(
		byte[] firmwareImage,
		FirmwarePackageMetadata firmwarePackage,
		IProgress<OtaUploadProgress>? progress = null,
		CancellationToken cancellationToken = default)
	{
		ArgumentNullException.ThrowIfNull(firmwareImage);
		ArgumentNullException.ThrowIfNull(firmwarePackage);

		progress?.Report(new OtaUploadProgress(0, firmwareImage.Length, "Przygotowywanie emulatora OTA..."));
		var begin = await _core.StartOtaAsync(new OtaRequest("ble", firmwareImage.Length, firmwarePackage.Version, firmwarePackage.ProjectName), cancellationToken).ConfigureAwait(false);
		if (!begin.Success)
		{
			return new OtaUploadResult(false, begin.Code, begin.Message ?? "OTA failed.", 0);
		}

		progress?.Report(new OtaUploadProgress(firmwareImage.Length, firmwareImage.Length, "Symulacja zapisu obrazu..."));
		var finish = await _core.FinishOtaAsync(cancellationToken).ConfigureAwait(false);
		return new OtaUploadResult(finish.Success, finish.Code, finish.Message ?? "OTA complete.", 0);
	}

	public void Dispose()
	{
		if (_disposed)
		{
			return;
		}

		_disposed = true;
	}

	private async Task<AquariumStatus> ReadStatusCoreAsync(CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var status = await _core.ReadStatusAsync(cancellationToken).ConfigureAwait(false);
		return status.ToLegacyStatus();
	}

	private async Task<AquariumSettings> ReadSettingsCoreAsync(CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var settings = await _core.ReadSettingsAsync(cancellationToken).ConfigureAwait(false);
		return settings.ToLegacySettings();
	}

	private async Task<AquariumDeviceInfo> ReadDeviceInfoCoreAsync(CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var info = await _core.ReadDeviceInfoAsync(cancellationToken).ConfigureAwait(false);
		return info.ToLegacyDeviceInfo();
	}
}
