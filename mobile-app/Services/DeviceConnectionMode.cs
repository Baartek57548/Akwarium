namespace AquariumController.Mobile.Services;

public enum DeviceConnectionMode
{
	RealDevice = 0,
	Emulator = 1
}

public sealed class DeviceConnectionModeChangedEventArgs : EventArgs
{
	public DeviceConnectionModeChangedEventArgs(DeviceConnectionMode previousMode, DeviceConnectionMode currentMode)
	{
		PreviousMode = previousMode;
		CurrentMode = currentMode;
	}

	public DeviceConnectionMode PreviousMode { get; }

	public DeviceConnectionMode CurrentMode { get; }
}

public interface IDeviceModeService
{
	DeviceConnectionMode CurrentMode { get; }

	event EventHandler<DeviceConnectionModeChangedEventArgs>? ModeChanged;

	Task SetModeAsync(DeviceConnectionMode mode, CancellationToken cancellationToken = default);
}
