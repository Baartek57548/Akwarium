using System.Buffers.Binary;
using System.Text.Json;
using System.Text.Json.Serialization;
using Microsoft.Extensions.Logging;
using Microsoft.Maui.ApplicationModel;
using Plugin.BLE.Abstractions;
using Plugin.BLE;
using Plugin.BLE.Abstractions.Contracts;
using Plugin.BLE.Abstractions.EventArgs;
using Plugin.BLE.Abstractions.Exceptions;

namespace AquariumController.Mobile.Services;

public sealed class BluetoothService : IBluetoothService, IDisposable
{
	private static readonly JsonSerializerOptions SerializerOptions = new(JsonSerializerDefaults.Web)
	{
		PropertyNameCaseInsensitive = true,
		DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
	};

	private readonly ILogger<BluetoothService> _logger;
	private readonly object _initializationLock = new();
	private readonly SemaphoreSlim _operationGate = new(1, 1);
	private readonly Dictionary<Guid, IDevice> _knownDevices = new();

	private IBluetoothLE? _bluetoothLe;
	private IAdapter? _adapter;
	private IDevice? _connectedDevice;
	private bool _isInitialized;
	private bool _disposed;
	private int _negotiatedMtu = 23;

	public BluetoothService(ILogger<BluetoothService> logger)
	{
		_logger = logger;
	}

	public event EventHandler<BluetoothAdapterStateChangedEventArgs>? AdapterStateChanged;

	public event EventHandler<BluetoothConnectionChangedEventArgs>? ConnectionChanged;

	public BleAdapterStatus AdapterState => _bluetoothLe is null ? BleAdapterStatus.Unknown : MapAdapterState(_bluetoothLe.State);

	public bool IsScanning => _adapter?.IsScanning ?? false;

	public bool IsConnected => _connectedDevice is not null;

	public BleDeviceInfo? ConnectedDevice => _connectedDevice is null ? null : MapDevice(_connectedDevice);

	public async Task<bool> EnsurePermissionsAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		if (DeviceInfo.Platform != DevicePlatform.Android)
		{
			EnsureInitialized();
			return true;
		}

		try
		{
#if ANDROID
			if (OperatingSystem.IsAndroidVersionAtLeast(31))
			{
				var scanGranted = await EnsurePermissionAsync<BluetoothScanPermission>();
				var connectGranted = await EnsurePermissionAsync<BluetoothConnectPermission>();
				return scanGranted && connectGranted;
			}

			return await EnsurePermissionAsync<Permissions.LocationWhenInUse>();
#else
			return true;
#endif
		}
		catch (Exception ex)
		{
			_logger.LogError(ex, "Failed to request Bluetooth permissions.");
			return false;
		}
	}

	public async Task<IReadOnlyList<BleDeviceInfo>> ScanForDevicesAsync(CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureBleReadyAsync(cancellationToken);
			_knownDevices.Clear();
			var adapter = GetAdapter();

			using var cancellationRegistration = cancellationToken.Register(() => _ = StopScanningAsync());

			if (adapter.IsScanning)
			{
				await adapter.StopScanningForDevicesAsync();
			}

			await adapter.StartScanningForDevicesAsync();

			return _knownDevices.Values
				.Select(MapDevice)
				.OrderByDescending(device => device.Rssi)
				.ToList();
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Scanning BLE devices failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task StopScanningAsync(CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		var adapter = _adapter;
		if (adapter?.IsScanning == true)
		{
			try
			{
				await adapter.StopScanningForDevicesAsync();
			}
			catch (Exception ex)
			{
				_logger.LogWarning(ex, "Stopping BLE scan failed.");
			}
		}
	}

	public async Task ConnectAsync(Guid deviceId, CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureBleReadyAsync(cancellationToken);
			await StopScanningAsync(cancellationToken);

			if (_connectedDevice?.Id == deviceId)
			{
				return;
			}

			if (_connectedDevice is not null)
			{
				await DisconnectDeviceCoreAsync();
			}

			if (_knownDevices.TryGetValue(deviceId, out var scannedDevice))
			{
				await GetAdapter().ConnectToDeviceAsync(scannedDevice);
				_connectedDevice = scannedDevice;
			}
			else
			{
				_connectedDevice = await GetAdapter().ConnectToKnownDeviceAsync(deviceId, cancellationToken: cancellationToken);
			}

			await WarmUpGattSessionAsync(cancellationToken);
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Connecting to Aquarium Controller failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task DisconnectAsync(CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			cancellationToken.ThrowIfCancellationRequested();
			await DisconnectDeviceCoreAsync();
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Disconnecting from Aquarium Controller failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<byte[]> ReadCharacteristicAsync(
		Guid serviceUuid,
		Guid characteristicUuid,
		CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var characteristic = await GetCharacteristicAsync(serviceUuid, characteristicUuid, cancellationToken);
			var readResult = await characteristic.ReadAsync();
			return readResult.data;
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException($"Reading BLE characteristic {characteristicUuid} failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task WriteCharacteristicAsync(
		Guid serviceUuid,
		Guid characteristicUuid,
		byte[] data,
		CancellationToken cancellationToken = default)
	{
		ArgumentNullException.ThrowIfNull(data);

		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var characteristic = await GetCharacteristicAsync(serviceUuid, characteristicUuid, cancellationToken);
			await MainThread.InvokeOnMainThreadAsync(() => characteristic.WriteAsync(data));
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException($"Writing BLE characteristic {characteristicUuid} failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<AquariumStatus> ReadStatusAsync(CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var payload = await ReadCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.StatusCharacteristicUuid,
				cancellationToken);

			return DeserializePayload<AquariumStatus>(payload) ?? AquariumStatus.Empty;
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Reading aquarium status failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<AquariumSettings> ReadSettingsAsync(CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var payload = await ReadCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.SettingsCharacteristicUuid,
				cancellationToken);

			return DeserializePayload<AquariumSettings>(payload) ?? new AquariumSettings();
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Reading aquarium settings failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<AquariumDeviceInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default)
	{
		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var payload = await ReadCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.DeviceInfoCharacteristicUuid,
				cancellationToken);

			return DeserializePayload<AquariumDeviceInfo>(payload) ?? AquariumDeviceInfo.Empty;
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Reading firmware information failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<AquariumCommandResult> SendCommandAsync(AquariumCommand command, CancellationToken cancellationToken = default)
	{
		ArgumentNullException.ThrowIfNull(command);

		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var payload = JsonSerializer.SerializeToUtf8Bytes(command.ToPayload(), SerializerOptions);

			await WriteCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.CommandCharacteristicUuid,
				payload,
				cancellationToken);

			return await ReadResultCoreAsync(cancellationToken);
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException($"Sending aquarium command '{command.Action}' failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<OtaUploadResult> UploadFirmwareAsync(
		byte[] firmwareImage,
		FirmwarePackageMetadata firmwarePackage,
		IProgress<OtaUploadProgress>? progress = null,
		CancellationToken cancellationToken = default)
	{
		ArgumentNullException.ThrowIfNull(firmwareImage);
		ArgumentNullException.ThrowIfNull(firmwarePackage);

		if (firmwareImage.Length == 0)
		{
			throw new InvalidOperationException("Wybrany plik firmware jest pusty.");
		}

		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);

			progress?.Report(new OtaUploadProgress(0, firmwareImage.Length, "Przygotowywanie sesji BLE OTA..."));

			var beginState = await SendOtaControlRequestCoreAsync(
				new AquariumOtaControlRequest(
					"begin",
					firmwareImage.Length,
					firmwarePackage.Version,
					firmwarePackage.ProjectName),
				cancellationToken);

			if (!string.Equals(beginState.Code, "ota_ready", StringComparison.OrdinalIgnoreCase))
			{
				throw new InvalidOperationException(MapOtaStateToMessage(beginState));
			}

			var chunkSize = ResolveOtaChunkSize(beginState.RecommendedChunkSizeBytes);
			progress?.Report(new OtaUploadProgress(0, firmwareImage.Length, $"Wysyłanie danych BLE paczkami po {chunkSize} B..."));

			for (var offset = 0; offset < firmwareImage.Length; offset += chunkSize)
			{
				cancellationToken.ThrowIfCancellationRequested();

				var bytesToSend = Math.Min(chunkSize, firmwareImage.Length - offset);
				var chunkPayload = new byte[bytesToSend + 4];
				BinaryPrimitives.WriteUInt32LittleEndian(chunkPayload.AsSpan(0, 4), (uint)offset);
				firmwareImage.AsSpan(offset, bytesToSend).CopyTo(chunkPayload.AsSpan(4));

				await WriteCharacteristicCoreAsync(
					AquariumBleContract.ServiceUuid,
					AquariumBleContract.OtaDataCharacteristicUuid,
					chunkPayload,
					cancellationToken);

				progress?.Report(new OtaUploadProgress(offset + bytesToSend, firmwareImage.Length, "Zapisywanie firmware do pamięci flash..."));
			}

			var finishState = await SendOtaControlRequestCoreAsync(
				new AquariumOtaControlRequest("finish"),
				cancellationToken);

			if (!string.Equals(finishState.Code, "ota_complete", StringComparison.OrdinalIgnoreCase))
			{
				throw new InvalidOperationException(MapOtaStateToMessage(finishState));
			}

			progress?.Report(new OtaUploadProgress(
				firmwareImage.Length,
				firmwareImage.Length,
				"Aktualizacja zakończona. Sterownik przełącza się na nowy obraz."));

			return new OtaUploadResult(
				true,
				finishState.Code,
				MapOtaStateToMessage(finishState),
				finishState.RebootDelayMilliseconds);
		}
		catch (OperationCanceledException)
		{
			await TryAbortOtaSessionAsync();
			throw;
		}
		catch (Exception ex)
		{
			await TryAbortOtaSessionAsync();
			throw WrapException("Wgrywanie firmware przez BLE nie powiodło się.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public async Task<AquariumCommandResult> SaveSettingsAsync(
		AquariumSettings settings,
		CancellationToken cancellationToken = default)
	{
		ArgumentNullException.ThrowIfNull(settings);

		await _operationGate.WaitAsync(cancellationToken);
		try
		{
			await EnsureConnectedAsync(cancellationToken);
			var payload = JsonSerializer.SerializeToUtf8Bytes(settings.ToBlePayload(), SerializerOptions);

			await WriteCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.SettingsCharacteristicUuid,
				payload,
				cancellationToken);

			return await ReadResultCoreAsync(cancellationToken);
		}
		catch (OperationCanceledException)
		{
			throw;
		}
		catch (Exception ex)
		{
			throw WrapException("Saving aquarium settings failed.", ex);
		}
		finally
		{
			_operationGate.Release();
		}
	}

	public void Dispose()
	{
		if (_disposed)
		{
			return;
		}

		if (_isInitialized && _adapter is not null && _bluetoothLe is not null)
		{
			_adapter.DeviceDiscovered -= OnDeviceDiscovered;
			_adapter.DeviceConnected -= OnDeviceConnected;
			_adapter.DeviceDisconnected -= OnDeviceDisconnected;
			_adapter.DeviceConnectionLost -= OnDeviceConnectionLost;
			_bluetoothLe.StateChanged -= OnBluetoothStateChanged;
		}

		_operationGate.Dispose();
		_disposed = true;
	}

	private async Task EnsureBleReadyAsync(CancellationToken cancellationToken)
	{
		EnsureInitialized();

		if (!await EnsurePermissionsAsync(cancellationToken))
		{
			throw new InvalidOperationException("Bluetooth permissions are not granted.");
		}

		if (AdapterState != BleAdapterStatus.On)
		{
			throw new InvalidOperationException("Bluetooth adapter is not enabled.");
		}
	}

	private void EnsureInitialized()
	{
		if (_isInitialized)
		{
			return;
		}

		lock (_initializationLock)
		{
			if (_isInitialized)
			{
				return;
			}

			_bluetoothLe = CrossBluetoothLE.Current;
			_adapter = _bluetoothLe.Adapter;
			_adapter.ScanTimeout = (int)TimeSpan.FromSeconds(10).TotalMilliseconds;
			_adapter.DeviceDiscovered += OnDeviceDiscovered;
			_adapter.DeviceConnected += OnDeviceConnected;
			_adapter.DeviceDisconnected += OnDeviceDisconnected;
			_adapter.DeviceConnectionLost += OnDeviceConnectionLost;
			_bluetoothLe.StateChanged += OnBluetoothStateChanged;
			_isInitialized = true;
		}
	}

	private IAdapter GetAdapter()
	{
		EnsureInitialized();
		return _adapter ?? throw new InvalidOperationException("Bluetooth adapter is not initialized.");
	}

	private async Task EnsureConnectedAsync(CancellationToken cancellationToken)
	{
		await EnsureBleReadyAsync(cancellationToken);

		if (_connectedDevice is null)
		{
			throw new InvalidOperationException("No Aquarium Controller is connected.");
		}
	}

	private async Task DisconnectDeviceCoreAsync()
	{
		if (_connectedDevice is null)
		{
			return;
		}

		await GetAdapter().DisconnectDeviceAsync(_connectedDevice);
		_connectedDevice = null;
		_negotiatedMtu = 23;
	}

	private async Task WarmUpGattSessionAsync(CancellationToken cancellationToken)
	{
		try
		{
			_negotiatedMtu = await _connectedDevice!.RequestMtuAsync(185, cancellationToken);
		}
		catch (Exception ex)
		{
			_logger.LogDebug(ex, "BLE MTU negotiation was not applied. Continuing with platform defaults.");
		}

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.StatusCharacteristicUuid,
			cancellationToken);

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.CommandCharacteristicUuid,
			cancellationToken);

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.ResultCharacteristicUuid,
			cancellationToken);

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.DeviceInfoCharacteristicUuid,
			cancellationToken);

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.OtaControlCharacteristicUuid,
			cancellationToken);

		await GetCharacteristicAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.OtaDataCharacteristicUuid,
			cancellationToken);
	}

	private async Task<byte[]> ReadCharacteristicCoreAsync(
		Guid serviceUuid,
		Guid characteristicUuid,
		CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var characteristic = await GetCharacteristicAsync(serviceUuid, characteristicUuid, cancellationToken);
		var readResult = await characteristic.ReadAsync();
		return readResult.data;
	}

	private async Task WriteCharacteristicCoreAsync(
		Guid serviceUuid,
		Guid characteristicUuid,
		byte[] data,
		CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		var characteristic = await GetCharacteristicAsync(serviceUuid, characteristicUuid, cancellationToken);
		await MainThread.InvokeOnMainThreadAsync(() => characteristic.WriteAsync(data));
	}

	private async Task<AquariumCommandResult> ReadResultCoreAsync(CancellationToken cancellationToken)
	{
		await Task.Delay(200, cancellationToken);

		var payload = await ReadCharacteristicCoreAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.ResultCharacteristicUuid,
			cancellationToken);

		return DeserializePayload<AquariumCommandResult>(payload) ?? AquariumCommandResult.Empty;
	}

	private async Task<AquariumOtaState> SendOtaControlRequestCoreAsync(
		AquariumOtaControlRequest request,
		CancellationToken cancellationToken)
	{
		var payload = JsonSerializer.SerializeToUtf8Bytes(request, SerializerOptions);

		await WriteCharacteristicCoreAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.OtaControlCharacteristicUuid,
			payload,
			cancellationToken);

		return await ReadOtaStateCoreAsync(cancellationToken);
	}

	private async Task<AquariumOtaState> ReadOtaStateCoreAsync(CancellationToken cancellationToken)
	{
		await Task.Delay(120, cancellationToken);

		var payload = await ReadCharacteristicCoreAsync(
			AquariumBleContract.ServiceUuid,
			AquariumBleContract.OtaControlCharacteristicUuid,
			cancellationToken);

		return DeserializePayload<AquariumOtaState>(payload) ?? AquariumOtaState.Empty;
	}

	private async Task TryAbortOtaSessionAsync()
	{
		try
		{
			if (_connectedDevice is null)
			{
				return;
			}

			var request = new AquariumOtaControlRequest("abort");
			var payload = JsonSerializer.SerializeToUtf8Bytes(request, SerializerOptions);
			await WriteCharacteristicCoreAsync(
				AquariumBleContract.ServiceUuid,
				AquariumBleContract.OtaControlCharacteristicUuid,
				payload,
				CancellationToken.None);
		}
		catch (Exception ex)
		{
			_logger.LogDebug(ex, "BLE OTA abort request could not be delivered.");
		}
	}

	private int ResolveOtaChunkSize(int serverRecommendedChunkSize)
	{
		var chunkSize = serverRecommendedChunkSize > 0 ? serverRecommendedChunkSize : 160;
		if (_negotiatedMtu > 23)
		{
			chunkSize = Math.Min(chunkSize, Math.Max(20, _negotiatedMtu - 7));
		}

		return Math.Clamp(chunkSize, 20, 160);
	}

	private static string MapOtaStateToMessage(AquariumOtaState otaState)
	{
		return otaState.Code switch
		{
			"ready" => "Sterownik jest gotowy do OTA.",
			"ota_ready" => "Sesja BLE OTA została zainicjalizowana.",
			"ota_receiving" => "Firmware jest zapisywany do pamięci flash.",
			"ota_complete" => "Wgrywanie firmware zakończyło się powodzeniem.",
			"ota_busy" => "Na sterowniku jest już aktywna inna sesja OTA.",
			"ota_bad_size" => "Sterownik odrzucił rozmiar firmware.",
			"ota_too_large" => "Obraz firmware jest większy niż dostępna partycja OTA.",
			"ota_no_partition" => "Sterownik nie ma wolnej partycji OTA.",
			"ota_begin_failed" => "Sterownik nie rozpoczął sesji OTA.",
			"ota_not_started" => "Sterownik nie ma aktywnej sesji BLE OTA.",
			"ota_size_mismatch" => "Sterownik nadal oczekuje brakujących bajtów firmware.",
			"ota_end_failed" => "Sterownik odrzucił wgrany obraz podczas końcowej walidacji.",
			"ota_boot_failed" => "Sterownik nie mógł przełączyć partycji startowej na nowy obraz.",
			"ota_offset_mismatch" => "Sterownik otrzymał paczki BLE OTA w nieprawidłowej kolejności.",
			"ota_write_failed" => "Sterownik napotkał błąd podczas zapisu danych OTA do flash.",
			"ota_timeout" => "Sesja BLE OTA przekroczyła limit czasu przed zakończeniem transferu.",
			"ota_aborted" => "Sesja BLE OTA została przerwana.",
			"ota_overflow" => "Wysłane dane przekroczyły zadeklarowany rozmiar firmware.",
			"ota_empty_request" => "Sterownik otrzymał pustą komendę sterującą OTA.",
			"ota_empty_chunk" => "Sterownik odrzucił pustą paczkę OTA.",
			_ => $"{otaState.Type}: {otaState.Code}"
		};
	}

	private async Task<ICharacteristic> GetCharacteristicAsync(
		Guid serviceUuid,
		Guid characteristicUuid,
		CancellationToken cancellationToken)
	{
		cancellationToken.ThrowIfCancellationRequested();
		await EnsureConnectedAsync(cancellationToken);

		var service = await _connectedDevice!.GetServiceAsync(serviceUuid)
			?? throw new InvalidOperationException($"BLE service {serviceUuid} was not found.");

		return await service.GetCharacteristicAsync(characteristicUuid)
			?? throw new InvalidOperationException($"BLE characteristic {characteristicUuid} was not found.");
	}

	private static TPayload? DeserializePayload<TPayload>(byte[] payload)
	{
		if (payload.Length == 0)
		{
			return default;
		}

		return JsonSerializer.Deserialize<TPayload>(payload, SerializerOptions);
	}

	private void OnDeviceDiscovered(object? sender, DeviceEventArgs e)
	{
		if (e.Device is null || !LooksLikeAquariumController(e.Device))
		{
			return;
		}

		_knownDevices[e.Device.Id] = e.Device;
	}

	private void OnDeviceConnected(object? sender, DeviceEventArgs e)
	{
		if (e.Device is null)
		{
			return;
		}

		_knownDevices[e.Device.Id] = e.Device;
		_connectedDevice = e.Device;

		var mappedDevice = MapDevice(e.Device);
		ConnectionChanged?.Invoke(
			this,
			new BluetoothConnectionChangedEventArgs(
				BluetoothConnectionState.Connected,
				mappedDevice,
				$"Connected to {mappedDevice.DisplayName}."));
	}

	private void OnDeviceDisconnected(object? sender, DeviceEventArgs e)
	{
		if (e.Device is null)
		{
			return;
		}

		if (_connectedDevice?.Id == e.Device.Id)
		{
			_connectedDevice = null;
			_negotiatedMtu = 23;
		}

		var mappedDevice = MapDevice(e.Device);
		ConnectionChanged?.Invoke(
			this,
			new BluetoothConnectionChangedEventArgs(
				BluetoothConnectionState.Disconnected,
				mappedDevice,
				$"{mappedDevice.DisplayName} disconnected."));
	}

	private void OnDeviceConnectionLost(object? sender, DeviceErrorEventArgs e)
	{
		if (e.Device is null)
		{
			return;
		}

		if (_connectedDevice?.Id == e.Device.Id)
		{
			_connectedDevice = null;
			_negotiatedMtu = 23;
		}

		var mappedDevice = MapDevice(e.Device);
		ConnectionChanged?.Invoke(
			this,
			new BluetoothConnectionChangedEventArgs(
				BluetoothConnectionState.ConnectionLost,
				mappedDevice,
				$"Connection lost with {mappedDevice.DisplayName}."));
	}

	private void OnBluetoothStateChanged(object? sender, BluetoothStateChangedArgs e)
	{
		var previousState = MapAdapterState(e.OldState);
		var currentState = MapAdapterState(e.NewState);

		if (currentState != BleAdapterStatus.On)
		{
			_connectedDevice = null;
			_negotiatedMtu = 23;
		}

		AdapterStateChanged?.Invoke(
			this,
			new BluetoothAdapterStateChangedEventArgs(previousState, currentState));
	}

	private static bool LooksLikeAquariumController(IDevice device)
	{
		return !string.IsNullOrWhiteSpace(device.Name)
			&& device.Name.Contains("Akwarium", StringComparison.OrdinalIgnoreCase);
	}

	private static BleDeviceInfo MapDevice(IDevice device)
	{
		return new BleDeviceInfo(device.Id, device.Name ?? AquariumBleContract.DeviceName, device.Rssi);
	}

	private static BleAdapterStatus MapAdapterState(BluetoothState state)
	{
		return state switch
		{
			BluetoothState.On => BleAdapterStatus.On,
			BluetoothState.Off => BleAdapterStatus.Off,
			BluetoothState.TurningOn => BleAdapterStatus.TurningOn,
			BluetoothState.TurningOff => BleAdapterStatus.TurningOff,
			BluetoothState.Unavailable => BleAdapterStatus.Unavailable,
			_ => BleAdapterStatus.Unknown
		};
	}

	private Exception WrapException(string message, Exception exception)
	{
		_logger.LogError(exception, "{Message}", message);

		return exception switch
		{
			DeviceConnectionException => new InvalidOperationException(
				$"{message} Pair the device in the OS prompt and verify the BLE PIN configured in firmware.",
				exception),
			CharacteristicReadException => new InvalidOperationException($"{message} Device rejected the read request.", exception),
			_ => new InvalidOperationException(message, exception)
		};
	}

#if ANDROID
	[System.Runtime.Versioning.SupportedOSPlatform("android31.0")]
	private sealed class BluetoothScanPermission : Permissions.BasePlatformPermission
	{
		public override (string androidPermission, bool isRuntime)[] RequiredPermissions =>
		[
			(global::Android.Manifest.Permission.BluetoothScan, true)
		];
	}

	[System.Runtime.Versioning.SupportedOSPlatform("android31.0")]
	private sealed class BluetoothConnectPermission : Permissions.BasePlatformPermission
	{
		public override (string androidPermission, bool isRuntime)[] RequiredPermissions =>
		[
			(global::Android.Manifest.Permission.BluetoothConnect, true)
		];
	}

	private static async Task<bool> EnsurePermissionAsync<TPermission>()
		where TPermission : Permissions.BasePermission, new()
	{
		var currentStatus = await Permissions.CheckStatusAsync<TPermission>();
		if (currentStatus == PermissionStatus.Granted)
		{
			return true;
		}

		var requestedStatus = await MainThread.InvokeOnMainThreadAsync(() => Permissions.RequestAsync<TPermission>());
		return requestedStatus == PermissionStatus.Granted;
	}
#endif
}
