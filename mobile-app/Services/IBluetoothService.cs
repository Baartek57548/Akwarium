namespace AquariumController.Mobile.Services;

public interface IBluetoothService
{
	event EventHandler<BluetoothAdapterStateChangedEventArgs>? AdapterStateChanged;
	event EventHandler<BluetoothConnectionChangedEventArgs>? ConnectionChanged;

	BleAdapterStatus AdapterState { get; }
	bool IsScanning { get; }
	bool IsConnected { get; }
	BleDeviceInfo? ConnectedDevice { get; }

	Task<bool> EnsurePermissionsAsync(CancellationToken cancellationToken = default);
	Task<IReadOnlyList<BleDeviceInfo>> ScanForDevicesAsync(CancellationToken cancellationToken = default);
	Task StopScanningAsync(CancellationToken cancellationToken = default);
	Task ConnectAsync(Guid deviceId, CancellationToken cancellationToken = default);
	Task DisconnectAsync(CancellationToken cancellationToken = default);
	Task<byte[]> ReadCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, CancellationToken cancellationToken = default);
	Task WriteCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, byte[] data, CancellationToken cancellationToken = default);
	Task<AquariumStatus> ReadStatusAsync(CancellationToken cancellationToken = default);
	Task<AquariumSettings> ReadSettingsAsync(CancellationToken cancellationToken = default);
	Task<AquariumDeviceInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default);
	Task<AquariumCommandResult> SendCommandAsync(AquariumCommand command, CancellationToken cancellationToken = default);
	Task<AquariumCommandResult> SaveSettingsAsync(AquariumSettings settings, CancellationToken cancellationToken = default);
	Task<OtaUploadResult> UploadFirmwareAsync(
		byte[] firmwareImage,
		FirmwarePackageMetadata firmwarePackage,
		IProgress<OtaUploadProgress>? progress = null,
		CancellationToken cancellationToken = default);
}

public enum BleAdapterStatus
{
	Unknown,
	Unavailable,
	Off,
	TurningOn,
	On,
	TurningOff
}

public enum BluetoothConnectionState
{
	Disconnected,
	Connected,
	ConnectionLost
}

public sealed class BluetoothAdapterStateChangedEventArgs : EventArgs
{
	public BluetoothAdapterStateChangedEventArgs(BleAdapterStatus previousState, BleAdapterStatus currentState)
	{
		PreviousState = previousState;
		CurrentState = currentState;
	}

	public BleAdapterStatus PreviousState { get; }

	public BleAdapterStatus CurrentState { get; }
}

public sealed class BluetoothConnectionChangedEventArgs : EventArgs
{
	public BluetoothConnectionChangedEventArgs(
		BluetoothConnectionState state,
		BleDeviceInfo? device,
		string message)
	{
		State = state;
		Device = device;
		Message = message;
	}

	public BluetoothConnectionState State { get; }

	public BleDeviceInfo? Device { get; }

	public string Message { get; }
}

public sealed record BleDeviceInfo(Guid Id, string Name, int Rssi)
{
	public string DisplayName => string.IsNullOrWhiteSpace(Name) ? "Aquarium Controller" : Name;

	public string SignalDescription => Rssi == 0 ? "Signal n/a" : $"{Rssi} dBm";

	public string IdentifierSummary => $"ID {Id:D}";
}
