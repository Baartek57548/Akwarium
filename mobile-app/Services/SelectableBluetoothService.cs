using Microsoft.Extensions.Logging;

namespace AquariumController.Mobile.Services;

public sealed class SelectableBluetoothService : IBluetoothService, IDisposable
{
	private readonly BluetoothService _realService;
	private readonly EmulatorBluetoothService _emulatorService;
	private readonly IDeviceModeService _modeService;
	private bool _disposed;

	public SelectableBluetoothService(
		BluetoothService realService,
		EmulatorBluetoothService emulatorService,
		IDeviceModeService modeService)
	{
		_realService = realService;
		_emulatorService = emulatorService;
		_modeService = modeService;
	}

	public event EventHandler<BluetoothAdapterStateChangedEventArgs>? AdapterStateChanged
	{
		add
		{
			_realService.AdapterStateChanged += value;
			_emulatorService.AdapterStateChanged += value;
		}
		remove
		{
			_realService.AdapterStateChanged -= value;
			_emulatorService.AdapterStateChanged -= value;
		}
	}

	public event EventHandler<BluetoothConnectionChangedEventArgs>? ConnectionChanged
	{
		add
		{
			_realService.ConnectionChanged += value;
			_emulatorService.ConnectionChanged += value;
		}
		remove
		{
			_realService.ConnectionChanged -= value;
			_emulatorService.ConnectionChanged -= value;
		}
	}

	public BleAdapterStatus AdapterState => ActiveService.AdapterState;

	public bool IsScanning => ActiveService.IsScanning;

	public bool IsConnected => ActiveService.IsConnected;

	public BleDeviceInfo? ConnectedDevice => ActiveService.ConnectedDevice;

	public Task<bool> EnsurePermissionsAsync(CancellationToken cancellationToken = default) => ActiveService.EnsurePermissionsAsync(cancellationToken);

	public Task<IReadOnlyList<BleDeviceInfo>> ScanForDevicesAsync(CancellationToken cancellationToken = default) => ActiveService.ScanForDevicesAsync(cancellationToken);

	public Task StopScanningAsync(CancellationToken cancellationToken = default) => ActiveService.StopScanningAsync(cancellationToken);

	public Task ConnectAsync(Guid deviceId, CancellationToken cancellationToken = default) => ActiveService.ConnectAsync(deviceId, cancellationToken);

	public Task DisconnectAsync(CancellationToken cancellationToken = default) => ActiveService.DisconnectAsync(cancellationToken);

	public Task<byte[]> ReadCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, CancellationToken cancellationToken = default) => ActiveService.ReadCharacteristicAsync(serviceUuid, characteristicUuid, cancellationToken);

	public Task WriteCharacteristicAsync(Guid serviceUuid, Guid characteristicUuid, byte[] data, CancellationToken cancellationToken = default) => ActiveService.WriteCharacteristicAsync(serviceUuid, characteristicUuid, data, cancellationToken);

	public Task<AquariumStatus> ReadStatusAsync(CancellationToken cancellationToken = default) => ActiveService.ReadStatusAsync(cancellationToken);

	public Task<AquariumSettings> ReadSettingsAsync(CancellationToken cancellationToken = default) => ActiveService.ReadSettingsAsync(cancellationToken);

	public Task<AquariumDeviceInfo> ReadDeviceInfoAsync(CancellationToken cancellationToken = default) => ActiveService.ReadDeviceInfoAsync(cancellationToken);

	public Task<AquariumCommandResult> SendCommandAsync(AquariumCommand command, CancellationToken cancellationToken = default) => ActiveService.SendCommandAsync(command, cancellationToken);

	public Task<AquariumCommandResult> SaveSettingsAsync(AquariumSettings settings, CancellationToken cancellationToken = default) => ActiveService.SaveSettingsAsync(settings, cancellationToken);

	public Task<OtaUploadResult> UploadFirmwareAsync(
		byte[] firmwareImage,
		FirmwarePackageMetadata firmwarePackage,
		IProgress<OtaUploadProgress>? progress = null,
		CancellationToken cancellationToken = default) => ActiveService.UploadFirmwareAsync(firmwareImage, firmwarePackage, progress, cancellationToken);

	private IBluetoothService ActiveService => _modeService.CurrentMode == DeviceConnectionMode.Emulator ? _emulatorService : _realService;

	public void Dispose()
	{
		if (_disposed)
		{
			return;
		}

		_disposed = true;
		_realService.Dispose();
		_emulatorService.Dispose();
	}
}
