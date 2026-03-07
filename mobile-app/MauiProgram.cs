using AquariumController.Mobile.Services;
using AquariumController.Mobile.ViewModels;
using Microsoft.Extensions.Logging;
using Microsoft.Maui.Storage;

namespace AquariumController.Mobile;

public static class MauiProgram
{
	public static MauiApp CreateMauiApp()
	{
		var builder = MauiApp.CreateBuilder();
		builder
			.UseMauiApp<App>()
			.ConfigureFonts(fonts =>
			{
				fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
				fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
			});

		builder.Services.AddSingleton<IFilePicker>(FilePicker.Default);
		builder.Services.AddSingleton<IBluetoothService, BluetoothService>();
		builder.Services.AddSingleton<IFirmwarePackageService, FirmwarePackageService>();
		builder.Services.AddTransient<MainViewModel>();
		builder.Services.AddTransient<MainPage>();

#if DEBUG
		builder.Logging.AddDebug();
#endif

		return builder.Build();
	}
}
