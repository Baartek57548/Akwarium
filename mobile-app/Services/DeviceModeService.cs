using Microsoft.Maui.Storage;

namespace AquariumController.Mobile.Services;

public sealed class DeviceModeService : IDeviceModeService
{
	private const string PreferenceKey = "Aquarium.ConnectionMode";

	public event EventHandler<DeviceConnectionModeChangedEventArgs>? ModeChanged;

	public DeviceConnectionMode CurrentMode => Enum.TryParse<DeviceConnectionMode>(Preferences.Default.Get(PreferenceKey, DeviceConnectionMode.RealDevice.ToString()), out var mode)
		? mode
		: DeviceConnectionMode.RealDevice;

	public Task SetModeAsync(DeviceConnectionMode mode, CancellationToken cancellationToken = default)
	{
		cancellationToken.ThrowIfCancellationRequested();

		var previousMode = CurrentMode;
		Preferences.Default.Set(PreferenceKey, mode.ToString());

		if (previousMode != mode)
		{
			ModeChanged?.Invoke(this, new DeviceConnectionModeChangedEventArgs(previousMode, mode));
		}

		return Task.CompletedTask;
	}
}
