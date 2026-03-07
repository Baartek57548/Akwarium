using System.Collections.ObjectModel;
using System.Globalization;
using AquariumController.Mobile.Services;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using Microsoft.Maui.ApplicationModel;
using Microsoft.Maui.Dispatching;
using Microsoft.Maui.Graphics;

namespace AquariumController.Mobile.ViewModels;

public sealed class SelectionOption<T>
{
	public SelectionOption(T value, string label, string? helperText = null)
	{
		Value = value;
		Label = label;
		HelperText = helperText ?? string.Empty;
	}

	public T Value { get; }

	public string Label { get; }

	public string HelperText { get; }
}

public sealed class ActivityLogEntry
{
	public ActivityLogEntry(DateTimeOffset timestamp, string category, string message, bool isCritical)
	{
		Timestamp = timestamp;
		Category = category;
		Message = message;
		IsCritical = isCritical;
	}

	public DateTimeOffset Timestamp { get; }

	public string Category { get; }

	public string Message { get; }

	public bool IsCritical { get; }

	public string FormattedText => $"[{Timestamp.LocalDateTime:HH:mm:ss}] {Message}";

	public Color ColorHex => IsCritical ? Color.FromArgb("#FB7185") : Color.FromArgb("#67E8F9");
}

public sealed class FirmwarePackageCard
{
	public static FirmwarePackageCard Empty { get; } = new("-", "Brak wybranego pakietu", "-", "-", "Wybierz pakiet .bin.", false);

	public FirmwarePackageCard(
		string fileName,
		string fileSizeText,
		string versionText,
		string buildText,
		string validationText,
		bool canUpload)
	{
		FileName = fileName;
		FileSizeText = fileSizeText;
		VersionText = versionText;
		BuildText = buildText;
		ValidationText = validationText;
		CanUpload = canUpload;
	}

	public string FileName { get; }

	public string FileSizeText { get; }

	public string VersionText { get; }

	public string BuildText { get; }

	public string ValidationText { get; }

	public bool CanUpload { get; }
}

public sealed class MainViewModel : ObservableObject
{
	private readonly IBluetoothService _bluetoothService;
	private readonly IFirmwarePackageService _firmwarePackageService;
	private readonly List<ActivityLogEntry> _allLogs = [];
	private readonly IReadOnlyList<SelectionOption<int>> _hourOptions;
	private readonly IReadOnlyList<SelectionOption<AquariumScheduleMode>> _scheduleModes;
	private IDispatcherTimer? _clockTimer;
	private FirmwarePackageDescriptor? _selectedFirmwareDescriptor;
	private CancellationTokenSource? _otaCancellationSource;
	private bool _initialized;
	private bool _showCriticalLogsOnly;

	private AquariumStatus _currentStatus = AquariumStatus.Empty;
	private AquariumDeviceInfo _currentDeviceInfo = AquariumDeviceInfo.Empty;
	private bool _isConnected;
	private bool _isDashboardTabSelected = true;
	private bool _isLogsTabSelected;
	private bool _isSystemTabSelected;
	private bool _canSaveSchedules;
	private string _statusMessage = "Polacz kontroler przez BLE, aby odczytac status i harmonogramy.";
	private string _lastStatusUpdateText = "Brak odswiezenia.";
	private string _latestResultText = "Brak odpowiedzi sterownika.";
	private string _currentTimeText = DateTime.Now.ToString("HH:mm:ss");
	private string _scheduleValidationSummary = string.Empty;
	private string _lightScheduleErrorText = string.Empty;
	private string _filterScheduleErrorText = string.Empty;
	private string _aerationScheduleErrorText = string.Empty;
	private string _heaterScheduleErrorText = string.Empty;
	private string _feedScheduleErrorText = string.Empty;
	private Color _batteryFillColor = Color.FromArgb("#4ADE80");
	private double _otaProgress;
	private string _otaProgressText = "BLE OTA idle.";
	private string _otaStatusText = "Brak aktywnej aktualizacji.";
	private BleDeviceInfo? _selectedDevice;
	private FirmwarePackageCard _selectedFirmwarePackage = FirmwarePackageCard.Empty;
	private int _servoPercent;
	private int _servoPreOffMinutes = 30;
	private SelectionOption<AquariumScheduleMode>? _selectedLightMode;
	private SelectionOption<AquariumScheduleMode>? _selectedFilterMode;
	private SelectionOption<AquariumScheduleMode>? _selectedAerationMode;
	private SelectionOption<int>? _selectedDayStartHour;
	private SelectionOption<int>? _selectedDayStartMinute;
	private SelectionOption<int>? _selectedDayEndHour;
	private SelectionOption<int>? _selectedDayEndMinute;
	private SelectionOption<int>? _selectedFilterOnHour;
	private SelectionOption<int>? _selectedFilterOnMinute;
	private SelectionOption<int>? _selectedFilterOffHour;
	private SelectionOption<int>? _selectedFilterOffMinute;
	private SelectionOption<int>? _selectedAerationOnHour;
	private SelectionOption<int>? _selectedAerationOnMinute;
	private SelectionOption<int>? _selectedAerationOffHour;
	private SelectionOption<int>? _selectedAerationOffMinute;
	private SelectionOption<int?>? _selectedHeaterTarget;
	private SelectionOption<double>? _selectedHysteresisOption;
	private SelectionOption<int>? _selectedFeedHour;
	private SelectionOption<int>? _selectedFeedMinute;
	private SelectionOption<int>? _selectedFeedMode;
	private IReadOnlyList<SelectionOption<int>> _minuteOptions;
	private IReadOnlyList<SelectionOption<int?>> _heaterTargetOptions;
	private IReadOnlyList<SelectionOption<double>> _hysteresisOptions;
	private IReadOnlyList<SelectionOption<int>> _feedModes;

	public MainViewModel(
		IBluetoothService bluetoothService,
		IFirmwarePackageService firmwarePackageService)
	{
		_bluetoothService = bluetoothService;
		_firmwarePackageService = firmwarePackageService;

		_hourOptions = Enumerable.Range(0, 24)
			.Select(hour => new SelectionOption<int>(hour, hour.ToString("00", CultureInfo.InvariantCulture)))
			.ToArray();

		_scheduleModes =
		[
			new SelectionOption<AquariumScheduleMode>(AquariumScheduleMode.Schedule, "Harmonogram"),
			new SelectionOption<AquariumScheduleMode>(AquariumScheduleMode.AlwaysOn, "Zawsze wlaczone"),
			new SelectionOption<AquariumScheduleMode>(AquariumScheduleMode.AlwaysOff, "Zawsze wylaczone")
		];

		_minuteOptions = BuildMinuteOptions(AquariumValidationProfile.Default.MinuteStep);
		_heaterTargetOptions = BuildHeaterTargetOptions(AquariumValidationProfile.Default.Temperature);
		_hysteresisOptions = BuildHysteresisOptions(AquariumValidationProfile.Default.Hysteresis);
		_feedModes = BuildFeedModes(AquariumValidationProfile.Default.FeedModes);

		DiscoveredDevices = new ObservableCollection<BleDeviceInfo>();
		VisibleLogs = new ObservableCollection<ActivityLogEntry>();

		ShowDashboardTabCommand = new RelayCommand(() => SelectTab(nameof(IsDashboardTabSelected)));
		ShowLogsTabCommand = new RelayCommand(() => SelectTab(nameof(IsLogsTabSelected)));
		ShowSystemTabCommand = new RelayCommand(() => SelectTab(nameof(IsSystemTabSelected)));
		ShowSystemLogsCommand = new RelayCommand(() => ApplyLogFilter(false));
		ShowCriticalLogsCommand = new RelayCommand(() => ApplyLogFilter(true));
		ClearLogsViewCommand = new RelayCommand(ClearLogsView);
		CancelBleOtaCommand = new RelayCommand(CancelBleOta);

		RefreshStatusCommand = new AsyncRelayCommand(RefreshStatusAsync);
		SaveSchedulesCommand = new AsyncRelayCommand(SaveSchedulesAsync);
		ScanCommand = new AsyncRelayCommand(ScanAsync);
		ConnectCommand = new AsyncRelayCommand(ConnectAsync);
		DisconnectCommand = new AsyncRelayCommand(DisconnectAsync);
		FeedNowCommand = new AsyncRelayCommand(() => SendCommandAsync(AquariumCommand.FeedNow(), "Karmienie uruchomione."));
		ApplyServoSliderCommand = new AsyncRelayCommand(() => SendCommandAsync(AquariumCommand.SetServo(ServoPercentToAngle(_servoPercent)), "Nadpisanie serwa zastosowane."));
		OpenServoCommand = new AsyncRelayCommand(() => SendCommandAsync(AquariumCommand.SetServo(0), "Serwo ustawione na otwarte."));
		CloseServoCommand = new AsyncRelayCommand(() => SendCommandAsync(AquariumCommand.SetServo(90), "Serwo ustawione na zamkniete."));
		ClearServoOverrideCommand = new AsyncRelayCommand(() => SendCommandAsync(AquariumCommand.ClearServo(), "Nadpisanie serwa wyczyszczone."));
		ClearCriticalLogsCommand = new AsyncRelayCommand(ClearCriticalLogsAsync);
		PickFirmwarePackageCommand = new AsyncRelayCommand(PickFirmwarePackageAsync);
		StartBleOtaCommand = new AsyncRelayCommand(StartBleOtaAsync);

		_bluetoothService.AdapterStateChanged += HandleAdapterStateChanged;
		_bluetoothService.ConnectionChanged += HandleConnectionChanged;

		SelectedLightMode = _scheduleModes[0];
		SelectedFilterMode = _scheduleModes[0];
		SelectedAerationMode = _scheduleModes[0];
		SelectedDayStartHour = FindOption(_hourOptions, 10);
		SelectedDayStartMinute = FindOption(_minuteOptions, 0);
		SelectedDayEndHour = FindOption(_hourOptions, 21);
		SelectedDayEndMinute = FindOption(_minuteOptions, 30);
		SelectedFilterOnHour = FindOption(_hourOptions, 10);
		SelectedFilterOnMinute = FindOption(_minuteOptions, 30);
		SelectedFilterOffHour = FindOption(_hourOptions, 20);
		SelectedFilterOffMinute = FindOption(_minuteOptions, 30);
		SelectedAerationOnHour = FindOption(_hourOptions, 10);
		SelectedAerationOnMinute = FindOption(_minuteOptions, 0);
		SelectedAerationOffHour = FindOption(_hourOptions, 21);
		SelectedAerationOffMinute = FindOption(_minuteOptions, 0);
		SelectedHeaterTarget = FindOption(_heaterTargetOptions, 25);
		SelectedHysteresisOption = FindOption(_hysteresisOptions, 0.5);
		SelectedFeedHour = FindOption(_hourOptions, 18);
		SelectedFeedMinute = FindOption(_minuteOptions, 0);
		SelectedFeedMode = FindOption(_feedModes, 1);

		AddLog("system", "Panel BLE uruchomiony. Oczekiwanie na polaczenie.", false);
		AddLog("system", "Streaming logow firmware nie jest obecnie wystawiony przez kontrakt BLE. Widok pokazuje logi aplikacji i wyniki komend.", false);
	}

	public ObservableCollection<BleDeviceInfo> DiscoveredDevices { get; }

	public ObservableCollection<ActivityLogEntry> VisibleLogs { get; }

	public IReadOnlyList<SelectionOption<AquariumScheduleMode>> ScheduleModes => _scheduleModes;

	public IReadOnlyList<SelectionOption<int>> HourOptions => _hourOptions;

	public IReadOnlyList<SelectionOption<int>> MinuteOptions
	{
		get => _minuteOptions;
		private set => SetProperty(ref _minuteOptions, value);
	}

	public IReadOnlyList<SelectionOption<int?>> HeaterTargetOptions
	{
		get => _heaterTargetOptions;
		private set => SetProperty(ref _heaterTargetOptions, value);
	}

	public IReadOnlyList<SelectionOption<double>> HysteresisOptions
	{
		get => _hysteresisOptions;
		private set => SetProperty(ref _hysteresisOptions, value);
	}

	public IReadOnlyList<SelectionOption<int>> FeedModes
	{
		get => _feedModes;
		private set => SetProperty(ref _feedModes, value);
	}

	public AquariumStatus CurrentStatus
	{
		get => _currentStatus;
		private set
		{
			if (!SetProperty(ref _currentStatus, value))
			{
				return;
			}

			ServoPercent = value.ServoOpenPercent;
			BatteryFillColor = ResolveBatteryColor(value.BatteryPercent);

			OnPropertyChanged(nameof(BatteryProgress));
			OnPropertyChanged(nameof(BleConnectionHintText));
			OnPropertyChanged(nameof(ConnectionStateText));
			OnPropertyChanged(nameof(ConnectionStatusBadgeText));
			OnPropertyChanged(nameof(MinTemperatureText));
			OnPropertyChanged(nameof(MinTemperatureTimeText));
			OnPropertyChanged(nameof(NetworkSummaryText));
			OnPropertyChanged(nameof(ServoModeText));
			OnPropertyChanged(nameof(ServoPercentText));
			OnPropertyChanged(nameof(ConnectionClientsText));
		}
	}

	public AquariumDeviceInfo CurrentDeviceInfo
	{
		get => _currentDeviceInfo;
		private set
		{
			if (!SetProperty(ref _currentDeviceInfo, value))
			{
				return;
			}

			ApplyValidationProfile(value.ValidationProfile);

			OnPropertyChanged(nameof(CurrentControllerName));
			OnPropertyChanged(nameof(CurrentFirmwareVersionText));
			OnPropertyChanged(nameof(CurrentFirmwareBuildText));
			OnPropertyChanged(nameof(CurrentFirmwarePartitionText));
			OnPropertyChanged(nameof(CurrentFirmwareCapabilityText));
			OnPropertyChanged(nameof(OtaInfoText));
		}
	}

	public bool IsConnected
	{
		get => _isConnected;
		private set
		{
			if (!SetProperty(ref _isConnected, value))
			{
				return;
			}

			OnPropertyChanged(nameof(BleConnectionHintText));
			OnPropertyChanged(nameof(ConnectionStateText));
			OnPropertyChanged(nameof(ConnectionStatusBadgeText));
			OnPropertyChanged(nameof(SelectedDeviceSummary));
			ValidateSchedules();
		}
	}

	public bool IsDashboardTabSelected
	{
		get => _isDashboardTabSelected;
		private set => SetProperty(ref _isDashboardTabSelected, value);
	}

	public bool IsLogsTabSelected
	{
		get => _isLogsTabSelected;
		private set => SetProperty(ref _isLogsTabSelected, value);
	}

	public bool IsSystemTabSelected
	{
		get => _isSystemTabSelected;
		private set => SetProperty(ref _isSystemTabSelected, value);
	}

	public string StatusMessage
	{
		get => _statusMessage;
		private set => SetProperty(ref _statusMessage, value);
	}

	public string LastStatusUpdateText
	{
		get => _lastStatusUpdateText;
		private set => SetProperty(ref _lastStatusUpdateText, value);
	}

	public string LatestResultText
	{
		get => _latestResultText;
		private set => SetProperty(ref _latestResultText, value);
	}

	public string CurrentTimeText
	{
		get => _currentTimeText;
		private set => SetProperty(ref _currentTimeText, value);
	}

	public string ScheduleValidationSummary
	{
		get => _scheduleValidationSummary;
		private set
		{
			if (SetProperty(ref _scheduleValidationSummary, value))
			{
				OnPropertyChanged(nameof(HasScheduleValidationSummary));
			}
		}
	}

	public bool HasScheduleValidationSummary => !string.IsNullOrWhiteSpace(ScheduleValidationSummary);

	public string LightScheduleErrorText
	{
		get => _lightScheduleErrorText;
		private set
		{
			if (SetProperty(ref _lightScheduleErrorText, value))
			{
				OnPropertyChanged(nameof(HasLightScheduleError));
			}
		}
	}

	public bool HasLightScheduleError => !string.IsNullOrWhiteSpace(LightScheduleErrorText);

	public string FilterScheduleErrorText
	{
		get => _filterScheduleErrorText;
		private set
		{
			if (SetProperty(ref _filterScheduleErrorText, value))
			{
				OnPropertyChanged(nameof(HasFilterScheduleError));
			}
		}
	}

	public bool HasFilterScheduleError => !string.IsNullOrWhiteSpace(FilterScheduleErrorText);

	public string AerationScheduleErrorText
	{
		get => _aerationScheduleErrorText;
		private set
		{
			if (SetProperty(ref _aerationScheduleErrorText, value))
			{
				OnPropertyChanged(nameof(HasAerationScheduleError));
			}
		}
	}

	public bool HasAerationScheduleError => !string.IsNullOrWhiteSpace(AerationScheduleErrorText);

	public string HeaterScheduleErrorText
	{
		get => _heaterScheduleErrorText;
		private set
		{
			if (SetProperty(ref _heaterScheduleErrorText, value))
			{
				OnPropertyChanged(nameof(HasHeaterScheduleError));
			}
		}
	}

	public bool HasHeaterScheduleError => !string.IsNullOrWhiteSpace(HeaterScheduleErrorText);

	public string FeedScheduleErrorText
	{
		get => _feedScheduleErrorText;
		private set
		{
			if (SetProperty(ref _feedScheduleErrorText, value))
			{
				OnPropertyChanged(nameof(HasFeedScheduleError));
			}
		}
	}

	public bool HasFeedScheduleError => !string.IsNullOrWhiteSpace(FeedScheduleErrorText);

	public Color BatteryFillColor
	{
		get => _batteryFillColor;
		private set => SetProperty(ref _batteryFillColor, value);
	}

	public double BatteryProgress => Math.Clamp(CurrentStatus.BatteryPercent / 100d, 0d, 1d);

	public double OtaProgress
	{
		get => _otaProgress;
		private set
		{
			if (SetProperty(ref _otaProgress, value))
			{
				OnPropertyChanged(nameof(OtaProgressPercentText));
			}
		}
	}

	public string OtaProgressText
	{
		get => _otaProgressText;
		private set => SetProperty(ref _otaProgressText, value);
	}

	public string OtaStatusText
	{
		get => _otaStatusText;
		private set => SetProperty(ref _otaStatusText, value);
	}

	public string OtaProgressPercentText => $"{Math.Round(OtaProgress * 100d):0}%";

	public BleDeviceInfo? SelectedDevice
	{
		get => _selectedDevice;
		set
		{
			if (!SetProperty(ref _selectedDevice, value))
			{
				return;
			}

			OnPropertyChanged(nameof(SelectedDeviceSummary));
		}
	}

	public FirmwarePackageCard SelectedFirmwarePackage
	{
		get => _selectedFirmwarePackage;
		private set
		{
			if (!SetProperty(ref _selectedFirmwarePackage, value))
			{
				return;
			}

			OnPropertyChanged(nameof(SelectedFirmwareVersionText));
			OnPropertyChanged(nameof(SelectedFirmwareBuildText));
			OnPropertyChanged(nameof(SelectedFirmwareValidationText));
		}
	}

	public int ServoPercent
	{
		get => _servoPercent;
		set
		{
			value = Math.Clamp(value, 0, 100);
			if (SetProperty(ref _servoPercent, value))
			{
				OnPropertyChanged(nameof(ServoPercentText));
			}
		}
	}

	public SelectionOption<AquariumScheduleMode>? SelectedLightMode
	{
		get => _selectedLightMode;
		set
		{
			if (SetProperty(ref _selectedLightMode, value))
			{
				OnPropertyChanged(nameof(IsLightScheduleEditable));
				OnPropertyChanged(nameof(LightScheduleHintText));
				ValidateSchedules();
			}
		}
	}

	public SelectionOption<AquariumScheduleMode>? SelectedFilterMode
	{
		get => _selectedFilterMode;
		set
		{
			if (SetProperty(ref _selectedFilterMode, value))
			{
				OnPropertyChanged(nameof(IsFilterScheduleEditable));
				OnPropertyChanged(nameof(FilterScheduleHintText));
				ValidateSchedules();
			}
		}
	}

	public SelectionOption<AquariumScheduleMode>? SelectedAerationMode
	{
		get => _selectedAerationMode;
		set
		{
			if (SetProperty(ref _selectedAerationMode, value))
			{
				OnPropertyChanged(nameof(IsAerationScheduleEditable));
				OnPropertyChanged(nameof(AerationScheduleHintText));
				ValidateSchedules();
			}
		}
	}

	public SelectionOption<int>? SelectedDayStartHour
	{
		get => _selectedDayStartHour;
		set => SetScheduleSelection(ref _selectedDayStartHour, value);
	}

	public SelectionOption<int>? SelectedDayStartMinute
	{
		get => _selectedDayStartMinute;
		set => SetScheduleSelection(ref _selectedDayStartMinute, value);
	}

	public SelectionOption<int>? SelectedDayEndHour
	{
		get => _selectedDayEndHour;
		set => SetScheduleSelection(ref _selectedDayEndHour, value);
	}

	public SelectionOption<int>? SelectedDayEndMinute
	{
		get => _selectedDayEndMinute;
		set => SetScheduleSelection(ref _selectedDayEndMinute, value);
	}

	public SelectionOption<int>? SelectedFilterOnHour
	{
		get => _selectedFilterOnHour;
		set => SetScheduleSelection(ref _selectedFilterOnHour, value);
	}

	public SelectionOption<int>? SelectedFilterOnMinute
	{
		get => _selectedFilterOnMinute;
		set => SetScheduleSelection(ref _selectedFilterOnMinute, value);
	}

	public SelectionOption<int>? SelectedFilterOffHour
	{
		get => _selectedFilterOffHour;
		set => SetScheduleSelection(ref _selectedFilterOffHour, value);
	}

	public SelectionOption<int>? SelectedFilterOffMinute
	{
		get => _selectedFilterOffMinute;
		set => SetScheduleSelection(ref _selectedFilterOffMinute, value);
	}

	public SelectionOption<int>? SelectedAerationOnHour
	{
		get => _selectedAerationOnHour;
		set => SetScheduleSelection(ref _selectedAerationOnHour, value);
	}

	public SelectionOption<int>? SelectedAerationOnMinute
	{
		get => _selectedAerationOnMinute;
		set => SetScheduleSelection(ref _selectedAerationOnMinute, value);
	}

	public SelectionOption<int>? SelectedAerationOffHour
	{
		get => _selectedAerationOffHour;
		set => SetScheduleSelection(ref _selectedAerationOffHour, value);
	}

	public SelectionOption<int>? SelectedAerationOffMinute
	{
		get => _selectedAerationOffMinute;
		set => SetScheduleSelection(ref _selectedAerationOffMinute, value);
	}

	public SelectionOption<int?>? SelectedHeaterTarget
	{
		get => _selectedHeaterTarget;
		set
		{
			if (SetProperty(ref _selectedHeaterTarget, value))
			{
				OnPropertyChanged(nameof(TargetTemperatureText));
				ValidateSchedules();
			}
		}
	}

	public SelectionOption<double>? SelectedHysteresisOption
	{
		get => _selectedHysteresisOption;
		set
		{
			if (SetProperty(ref _selectedHysteresisOption, value))
			{
				OnPropertyChanged(nameof(TemperatureHysteresisText));
				ValidateSchedules();
			}
		}
	}

	public SelectionOption<int>? SelectedFeedHour
	{
		get => _selectedFeedHour;
		set => SetScheduleSelection(ref _selectedFeedHour, value);
	}

	public SelectionOption<int>? SelectedFeedMinute
	{
		get => _selectedFeedMinute;
		set => SetScheduleSelection(ref _selectedFeedMinute, value);
	}

	public SelectionOption<int>? SelectedFeedMode
	{
		get => _selectedFeedMode;
		set => SetScheduleSelection(ref _selectedFeedMode, value);
	}

	public bool IsLightScheduleEditable => SelectedLightMode?.Value == AquariumScheduleMode.Schedule;

	public bool IsFilterScheduleEditable => SelectedFilterMode?.Value == AquariumScheduleMode.Schedule;

	public bool IsAerationScheduleEditable => SelectedAerationMode?.Value == AquariumScheduleMode.Schedule;

	public string LightScheduleHintText => IsLightScheduleEditable
		? "Godziny aktywne tylko w trybie Harmonogram. Minuty sa ograniczone do kroku 5 minut."
		: "Tryb staly wylacza edycje godzin.";

	public string FilterScheduleHintText => IsFilterScheduleEditable
		? "Filtr pracuje wedlug wlasnego okna czasu."
		: "Filtr jest niezalezny od swiatla i ma tryb staly.";

	public string AerationScheduleHintText => IsAerationScheduleEditable
		? "Napowietrzanie ma niezalezny harmonogram."
		: "Napowietrzanie pracuje w trybie stalym.";

	public string BleConnectionHintText
	{
		get
		{
			if (IsConnected)
			{
				return $"Polaczono z {CurrentControllerName}. Profil walidacji pochodzi z firmware.";
			}

			if (SelectedDevice is not null)
			{
				return $"Wybrane urzadzenie: {SelectedDevice.DisplayName}. Uzyj Polacz, aby zsynchronizowac status.";
			}

			return "Zeskanuj urzadzenia BLE i polacz kontroler, aby odczytac status, harmonogramy i profil walidacji.";
		}
	}

	public string AdapterStateText => $"Adapter BLE: {MapAdapterState(_bluetoothService.AdapterState)}";

	public string ConnectionStateText => IsConnected ? "Polaczono" : "Rozlaczono";

	public string ConnectionStatusBadgeText => IsConnected ? "ONLINE" : "OFFLINE";

	public string NetworkSummaryText => CurrentStatus.NetworkText;

	public string MinTemperatureText => CurrentStatus.MinimumTemperatureText;

	public string MinTemperatureTimeText => CurrentStatus.MinimumTemperatureTimeText;

	public string ServoPercentText => $"{ServoPercent}%";

	public string ServoModeText => CurrentStatus.ServoModeText;

	public string SelectedDeviceSummary
	{
		get
		{
			if (IsConnected && _bluetoothService.ConnectedDevice is not null)
			{
				return $"{_bluetoothService.ConnectedDevice.DisplayName} ({_bluetoothService.ConnectedDevice.SignalDescription})";
			}

			if (SelectedDevice is not null)
			{
				return $"{SelectedDevice.DisplayName} ({SelectedDevice.SignalDescription})";
			}

			return "Brak wybranego urzadzenia.";
		}
	}

	public string CurrentControllerName => string.IsNullOrWhiteSpace(CurrentDeviceInfo.FirmwareName)
		? (_bluetoothService.ConnectedDevice?.DisplayName ?? "Aquarium Controller")
		: CurrentDeviceInfo.FirmwareName;

	public string ConnectionClientsText => $"Klienci BLE/AP: {CurrentStatus.ConnectedClients}";

	public string CurrentFirmwareVersionText => CurrentDeviceInfo.FirmwareDisplayText;

	public string CurrentFirmwareBuildText => CurrentDeviceInfo.BuildDisplayText;

	public string CurrentFirmwarePartitionText => $"{CurrentDeviceInfo.RunningPartition} -> {CurrentDeviceInfo.NextPartition}";

	public string CurrentFirmwareCapabilityText => $"{CurrentDeviceInfo.CompatibilitySummary}; slot {CurrentDeviceInfo.OtaSlotSizeText}";

	public string SelectedFirmwareVersionText => SelectedFirmwarePackage.VersionText;

	public string SelectedFirmwareBuildText => SelectedFirmwarePackage.BuildText;

	public string SelectedFirmwareValidationText => SelectedFirmwarePackage.ValidationText;

	public string OtaInfoText => $"Transport: {CurrentDeviceInfo.OtaTransportText}. Zalecany chunk: {CurrentDeviceInfo.RecommendedChunkSizeBytes} B. {CurrentDeviceInfo.CompatibilitySummary}.";

	public string TargetTemperatureText => SelectedHeaterTarget?.Value is null
		? "OFF"
		: $"{SelectedHeaterTarget.Value.Value:0} C";

	public string TemperatureHysteresisText => SelectedHysteresisOption is null
		? "-"
		: $"{SelectedHysteresisOption.Value:0.0} C";

	public bool CanSaveSchedules
	{
		get => _canSaveSchedules;
		private set => SetProperty(ref _canSaveSchedules, value);
	}

	public IRelayCommand ShowDashboardTabCommand { get; }

	public IRelayCommand ShowLogsTabCommand { get; }

	public IRelayCommand ShowSystemTabCommand { get; }

	public IRelayCommand ShowSystemLogsCommand { get; }

	public IRelayCommand ShowCriticalLogsCommand { get; }

	public IRelayCommand ClearLogsViewCommand { get; }

	public IRelayCommand CancelBleOtaCommand { get; }

	public IAsyncRelayCommand RefreshStatusCommand { get; }

	public IAsyncRelayCommand SaveSchedulesCommand { get; }

	public IAsyncRelayCommand ScanCommand { get; }

	public IAsyncRelayCommand ConnectCommand { get; }

	public IAsyncRelayCommand DisconnectCommand { get; }

	public IAsyncRelayCommand FeedNowCommand { get; }

	public IAsyncRelayCommand ApplyServoSliderCommand { get; }

	public IAsyncRelayCommand OpenServoCommand { get; }

	public IAsyncRelayCommand CloseServoCommand { get; }

	public IAsyncRelayCommand ClearServoOverrideCommand { get; }

	public IAsyncRelayCommand ClearCriticalLogsCommand { get; }

	public IAsyncRelayCommand PickFirmwarePackageCommand { get; }

	public IAsyncRelayCommand StartBleOtaCommand { get; }

	public async Task InitializeAsync()
	{
		if (_initialized)
		{
			return;
		}

		_initialized = true;
		StartClock();
		UpdateConnectionState();
		ApplyLogFilter(_showCriticalLogsOnly);

		if (IsConnected)
		{
			await RefreshStatusAsync();
		}
	}

	private void StartClock()
	{
		CurrentTimeText = DateTime.Now.ToString("HH:mm:ss");
		var dispatcher = Application.Current?.Dispatcher;
		if (dispatcher is null)
		{
			return;
		}

		_clockTimer = dispatcher.CreateTimer();
		_clockTimer.Interval = TimeSpan.FromSeconds(1);
		_clockTimer.Tick += (_, _) => CurrentTimeText = DateTime.Now.ToString("HH:mm:ss");
		_clockTimer.Start();
	}

	private void SelectTab(string propertyName)
	{
		IsDashboardTabSelected = propertyName == nameof(IsDashboardTabSelected);
		IsLogsTabSelected = propertyName == nameof(IsLogsTabSelected);
		IsSystemTabSelected = propertyName == nameof(IsSystemTabSelected);
	}

	private async Task ScanAsync()
	{
		try
		{
			StatusMessage = "Skanowanie BLE...";
			var devices = await _bluetoothService.ScanForDevicesAsync();

			DiscoveredDevices.Clear();
			foreach (var device in devices)
			{
				DiscoveredDevices.Add(device);
			}

			SelectedDevice = DiscoveredDevices.FirstOrDefault();
			StatusMessage = devices.Count == 0
				? "Nie wykryto zadnego kontrolera BLE."
				: $"Wykryto {devices.Count} urzadzen BLE.";
			AddLog("system", StatusMessage, false);
		}
		catch (Exception ex)
		{
			HandleException("Skanowanie BLE nie powiodlo sie.", ex, true);
		}
	}

	private async Task ConnectAsync()
	{
		if (SelectedDevice is null)
		{
			StatusMessage = "Najpierw wybierz urzadzenie BLE.";
			AddLog("system", StatusMessage, false);
			return;
		}

		try
		{
			StatusMessage = $"Laczenie z {SelectedDevice.DisplayName}...";
			await _bluetoothService.ConnectAsync(SelectedDevice.Id);
			UpdateConnectionState();
			await RefreshStatusAsync();
		}
		catch (Exception ex)
		{
			HandleException("Polaczenie BLE nie powiodlo sie.", ex, true);
		}
	}

	private async Task DisconnectAsync()
	{
		try
		{
			await _bluetoothService.DisconnectAsync();
			UpdateConnectionState();
			StatusMessage = "Rozlaczono kontroler.";
			AddLog("system", StatusMessage, false);
		}
		catch (Exception ex)
		{
			HandleException("Rozlaczenie BLE nie powiodlo sie.", ex, true);
		}
	}

	private async Task RefreshStatusAsync()
	{
		if (!IsConnected)
		{
			StatusMessage = "Brak polaczenia BLE.";
			return;
		}

		try
		{
			StatusMessage = "Odczyt statusu, ustawien i profilu walidacji...";

			var status = await _bluetoothService.ReadStatusAsync();
			var settings = await _bluetoothService.ReadSettingsAsync();
			var deviceInfo = await _bluetoothService.ReadDeviceInfoAsync();

			CurrentDeviceInfo = deviceInfo;
			CurrentStatus = status;
			ApplySettings(settings);

			LastStatusUpdateText = $"Ostatni odczyt: {DateTime.Now:HH:mm:ss}";
			StatusMessage = "Status zsynchronizowany z kontrolerem.";
			AddLog("system", StatusMessage, false);
		}
		catch (Exception ex)
		{
			HandleException("Odczyt statusu nie powiodl sie.", ex, true);
		}
	}

	private async Task SaveSchedulesAsync()
	{
		ValidateSchedules();
		if (!CanSaveSchedules)
		{
			LatestResultText = string.IsNullOrWhiteSpace(ScheduleValidationSummary)
				? "Popraw walidacje formularza przed zapisem."
				: ScheduleValidationSummary;
			return;
		}

		try
		{
			var payload = BuildSettingsPayload();
			var result = await _bluetoothService.SaveSettingsAsync(payload);
			ProcessCommandResult(result, "Harmonogramy zapisane.");

			if (result.IsSuccess)
			{
				await RefreshStatusAsync();
			}
		}
		catch (Exception ex)
		{
			HandleException("Zapis harmonogramow nie powiodl sie.", ex, true);
		}
	}

	private async Task SendCommandAsync(AquariumCommand command, string successMessage)
	{
		if (!IsConnected)
		{
			StatusMessage = "Brak polaczenia BLE.";
			return;
		}

		try
		{
			var result = await _bluetoothService.SendCommandAsync(command);
			ProcessCommandResult(result, successMessage);
			if (result.IsSuccess)
			{
				await RefreshStatusAsync();
			}
		}
		catch (Exception ex)
		{
			HandleException("Komenda BLE nie powiodla sie.", ex, true);
		}
	}

	private async Task ClearCriticalLogsAsync()
	{
		await SendCommandAsync(AquariumCommand.ClearCriticalLogs(), "Krytyczne logi zostaly usuniete.");
	}

	private async Task PickFirmwarePackageAsync()
	{
		try
		{
			var descriptor = await _firmwarePackageService.PickFirmwarePackageAsync();
			if (descriptor is null)
			{
				return;
			}

			_selectedFirmwareDescriptor = descriptor;
			SelectedFirmwarePackage = new FirmwarePackageCard(
				descriptor.FileName,
				descriptor.FileSizeText,
				descriptor.Metadata.Version,
				descriptor.Metadata.BuildDisplayText,
				descriptor.Metadata.ValidationMessage,
				descriptor.Metadata.CanBeUploaded);

			OtaStatusText = "Pakiet gotowy do wgrania przez BLE.";
			AddLog("system", $"Wybrano pakiet firmware {descriptor.FileName}.", false);
		}
		catch (Exception ex)
		{
			HandleException("Nie udalo sie odczytac pakietu firmware.", ex, true);
		}
	}

	private async Task StartBleOtaAsync()
	{
		if (!IsConnected)
		{
			StatusMessage = "Brak polaczenia BLE.";
			return;
		}

		if (_selectedFirmwareDescriptor is null || !SelectedFirmwarePackage.CanUpload)
		{
			StatusMessage = "Wybierz poprawny pakiet firmware ESP32-S3.";
			return;
		}

		_otaCancellationSource?.Cancel();
		_otaCancellationSource = new CancellationTokenSource();
		OtaProgress = 0d;
		OtaStatusText = "Rozpoczynanie BLE OTA...";
		OtaProgressText = "Przygotowywanie transferu.";

		try
		{
			var progress = new Progress<OtaUploadProgress>(update =>
			{
				OtaProgress = update.Progress;
				OtaProgressText = update.Stage;
				OtaStatusText = $"BLE OTA: {update.BytesSent}/{update.TotalBytes} B";
			});

			var result = await _bluetoothService.UploadFirmwareAsync(
				_selectedFirmwareDescriptor.FirmwareImage,
				_selectedFirmwareDescriptor.Metadata,
				progress,
				_otaCancellationSource.Token);

			OtaStatusText = result.Message;
			LatestResultText = result.Message;
			AddLog("system", result.Message, !result.IsSuccess);
		}
		catch (OperationCanceledException)
		{
			OtaStatusText = "BLE OTA anulowane.";
			AddLog("system", OtaStatusText, true);
		}
		catch (Exception ex)
		{
			HandleException("BLE OTA nie powiodlo sie.", ex, true);
		}
		finally
		{
			_otaCancellationSource?.Dispose();
			_otaCancellationSource = null;
		}
	}

	private void CancelBleOta()
	{
		_otaCancellationSource?.Cancel();
	}

	private void ApplySettings(AquariumSettings settings)
	{
		SelectedLightMode = FindOption(_scheduleModes, settings.LightMode);
		SelectedFilterMode = FindOption(_scheduleModes, settings.FilterMode);
		SelectedAerationMode = FindOption(_scheduleModes, settings.AerationMode);

		SelectedDayStartHour = FindOption(_hourOptions, settings.DayStartHour);
		SelectedDayStartMinute = FindOption(_minuteOptions, settings.DayStartMinute);
		SelectedDayEndHour = FindOption(_hourOptions, settings.DayEndHour);
		SelectedDayEndMinute = FindOption(_minuteOptions, settings.DayEndMinute);

		SelectedFilterOnHour = FindOption(_hourOptions, settings.FilterHourOn);
		SelectedFilterOnMinute = FindOption(_minuteOptions, settings.FilterMinuteOn);
		SelectedFilterOffHour = FindOption(_hourOptions, settings.FilterHourOff);
		SelectedFilterOffMinute = FindOption(_minuteOptions, settings.FilterMinuteOff);

		SelectedAerationOnHour = FindOption(_hourOptions, settings.AerationHourOn);
		SelectedAerationOnMinute = FindOption(_minuteOptions, settings.AerationMinuteOn);
		SelectedAerationOffHour = FindOption(_hourOptions, settings.AerationHourOff);
		SelectedAerationOffMinute = FindOption(_minuteOptions, settings.AerationMinuteOff);

		SelectedHeaterTarget = settings.HeaterMode == AquariumHeaterMode.Off
			? FindOption(_heaterTargetOptions, (int?)null)
			: FindOption(_heaterTargetOptions, (int?)Math.Round(settings.TargetTemperature));
		SelectedHysteresisOption = FindOption(_hysteresisOptions, Math.Round(settings.TemperatureHysteresis, 1));
		_servoPreOffMinutes = settings.ServoPreOffMinutes;

		SelectedFeedHour = FindOption(_hourOptions, settings.FeedHour);
		SelectedFeedMinute = FindOption(_minuteOptions, settings.FeedMinute);
		SelectedFeedMode = FindOption(_feedModes, settings.FeedMode);

		ValidateSchedules();
	}

	private AquariumSettings BuildSettingsPayload()
	{
		var heaterValue = SelectedHeaterTarget?.Value;
		var heaterMode = heaterValue is null ? AquariumHeaterMode.Off : AquariumHeaterMode.Threshold;
		var temperatureThreshold = heaterValue ?? ResolveDefaultThreshold();
		var hysteresisValue = SelectedHysteresisOption?.Value ?? ResolveDefaultHysteresis();

		return new AquariumSettings
		{
			LightModeCode = (int)(SelectedLightMode?.Value ?? AquariumScheduleMode.Schedule),
			DayStartHour = SelectedDayStartHour?.Value ?? 0,
			DayStartMinute = SelectedDayStartMinute?.Value ?? 0,
			DayEndHour = SelectedDayEndHour?.Value ?? 0,
			DayEndMinute = SelectedDayEndMinute?.Value ?? 0,
			FilterModeCode = (int)(SelectedFilterMode?.Value ?? AquariumScheduleMode.Schedule),
			FilterHourOn = SelectedFilterOnHour?.Value ?? 0,
			FilterMinuteOn = SelectedFilterOnMinute?.Value ?? 0,
			FilterHourOff = SelectedFilterOffHour?.Value ?? 0,
			FilterMinuteOff = SelectedFilterOffMinute?.Value ?? 0,
			AerationModeCode = (int)(SelectedAerationMode?.Value ?? AquariumScheduleMode.Schedule),
			AerationHourOn = SelectedAerationOnHour?.Value ?? 0,
			AerationMinuteOn = SelectedAerationOnMinute?.Value ?? 0,
			AerationHourOff = SelectedAerationOffHour?.Value ?? 0,
			AerationMinuteOff = SelectedAerationOffMinute?.Value ?? 0,
			HeaterModeCode = (int)heaterMode,
			TargetTemperature = temperatureThreshold,
			TemperatureHysteresis = hysteresisValue,
			FeedHour = SelectedFeedHour?.Value ?? 0,
			FeedMinute = SelectedFeedMinute?.Value ?? 0,
			FeedMode = SelectedFeedMode?.Value ?? 0,
			ServoPreOffMinutes = _servoPreOffMinutes
		};
	}

	private void ApplyValidationProfile(AquariumValidationProfile profile)
	{
		var previousMinutes = SelectedMinuteValues();
		var previousHeaterTarget = SelectedHeaterTarget?.Value;
		var previousHysteresis = SelectedHysteresisOption?.Value;
		var previousFeedMode = SelectedFeedMode?.Value;

		MinuteOptions = BuildMinuteOptions(profile.MinuteStep);
		HeaterTargetOptions = BuildHeaterTargetOptions(profile.Temperature);
		HysteresisOptions = BuildHysteresisOptions(profile.Hysteresis);
		FeedModes = BuildFeedModes(profile.FeedModes);

		RestoreMinuteSelections(previousMinutes);
		SelectedHeaterTarget = FindOption<int?>(HeaterTargetOptions, previousHeaterTarget) ?? HeaterTargetOptions.FirstOrDefault();
		SelectedHysteresisOption = FindOption<double>(HysteresisOptions, previousHysteresis ?? AquariumValidationProfile.Default.Hysteresis.Min) ?? HysteresisOptions.FirstOrDefault();
		SelectedFeedMode = FindOption<int>(FeedModes, previousFeedMode ?? AquariumValidationProfile.Default.FeedModes.Min) ?? FeedModes.FirstOrDefault();
	}

	private (int? dayStart, int? dayEnd, int? filterOn, int? filterOff, int? aerationOn, int? aerationOff, int? feed) SelectedMinuteValues()
	{
		return (
			SelectedDayStartMinute?.Value,
			SelectedDayEndMinute?.Value,
			SelectedFilterOnMinute?.Value,
			SelectedFilterOffMinute?.Value,
			SelectedAerationOnMinute?.Value,
			SelectedAerationOffMinute?.Value,
			SelectedFeedMinute?.Value);
	}

	private void RestoreMinuteSelections((int? dayStart, int? dayEnd, int? filterOn, int? filterOff, int? aerationOn, int? aerationOff, int? feed) snapshot)
	{
		SelectedDayStartMinute = FindOption<int>(MinuteOptions, snapshot.dayStart ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedDayEndMinute = FindOption<int>(MinuteOptions, snapshot.dayEnd ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedFilterOnMinute = FindOption<int>(MinuteOptions, snapshot.filterOn ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedFilterOffMinute = FindOption<int>(MinuteOptions, snapshot.filterOff ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedAerationOnMinute = FindOption<int>(MinuteOptions, snapshot.aerationOn ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedAerationOffMinute = FindOption<int>(MinuteOptions, snapshot.aerationOff ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
		SelectedFeedMinute = FindOption<int>(MinuteOptions, snapshot.feed ?? MinuteOptions.First().Value) ?? MinuteOptions.FirstOrDefault();
	}

	private void ValidateSchedules()
	{
		var summary = new List<string>();

		LightScheduleErrorText = ValidateScheduleCard(
			SelectedLightMode,
			SelectedDayStartHour,
			SelectedDayStartMinute,
			SelectedDayEndHour,
			SelectedDayEndMinute,
			"Oswietlenie");
		if (!string.IsNullOrWhiteSpace(LightScheduleErrorText))
		{
			summary.Add(LightScheduleErrorText);
		}

		FilterScheduleErrorText = ValidateScheduleCard(
			SelectedFilterMode,
			SelectedFilterOnHour,
			SelectedFilterOnMinute,
			SelectedFilterOffHour,
			SelectedFilterOffMinute,
			"Filtr");
		if (!string.IsNullOrWhiteSpace(FilterScheduleErrorText))
		{
			summary.Add(FilterScheduleErrorText);
		}

		AerationScheduleErrorText = ValidateScheduleCard(
			SelectedAerationMode,
			SelectedAerationOnHour,
			SelectedAerationOnMinute,
			SelectedAerationOffHour,
			SelectedAerationOffMinute,
			"Napowietrzanie");
		if (!string.IsNullOrWhiteSpace(AerationScheduleErrorText))
		{
			summary.Add(AerationScheduleErrorText);
		}

		HeaterScheduleErrorText = string.Empty;
		if (SelectedHeaterTarget is null)
		{
			HeaterScheduleErrorText = "Wybierz prog grzalki lub OFF.";
		}
		else if (SelectedHeaterTarget.Value is not null)
		{
			var tempRule = CurrentDeviceInfo.ValidationProfile.Temperature;
			var tempValue = SelectedHeaterTarget.Value.Value;
			if (tempValue < tempRule.Min || tempValue > tempRule.Max)
			{
				HeaterScheduleErrorText = $"Prog grzalki musi miescic sie w zakresie {tempRule.Min:0}-{tempRule.Max:0} C.";
			}
		}

		if (SelectedHysteresisOption is null)
		{
			HeaterScheduleErrorText = AppendMessage(HeaterScheduleErrorText, "Wybierz histereze.");
		}

		if (!string.IsNullOrWhiteSpace(HeaterScheduleErrorText))
		{
			summary.Add(HeaterScheduleErrorText);
		}

		FeedScheduleErrorText = string.Empty;
		if (SelectedFeedHour is null || SelectedFeedMinute is null)
		{
			FeedScheduleErrorText = "Karmienie wymaga poprawnej godziny.";
		}

		if (SelectedFeedMode is null)
		{
			FeedScheduleErrorText = AppendMessage(FeedScheduleErrorText, "Wybierz tryb karmienia.");
		}

		if (!string.IsNullOrWhiteSpace(FeedScheduleErrorText))
		{
			summary.Add(FeedScheduleErrorText);
		}

		ScheduleValidationSummary = summary.Count == 0
			? string.Empty
			: $"Popraw formularz przed zapisem. Pierwszy blad: {summary[0]}";

		CanSaveSchedules = IsConnected && summary.Count == 0;
	}

	private static string ValidateScheduleCard(
		SelectionOption<AquariumScheduleMode>? mode,
		SelectionOption<int>? startHour,
		SelectionOption<int>? startMinute,
		SelectionOption<int>? endHour,
		SelectionOption<int>? endMinute,
		string label)
	{
		if (mode is null)
		{
			return $"{label}: wybierz tryb pracy.";
		}

		if (mode.Value != AquariumScheduleMode.Schedule)
		{
			return string.Empty;
		}

		if (startHour is null || startMinute is null || endHour is null || endMinute is null)
		{
			return $"{label}: uzupelnij godziny start/stop.";
		}

		return string.Empty;
	}

	private void ProcessCommandResult(AquariumCommandResult result, string successMessage)
	{
		var translated = result.IsSuccess ? successMessage : TranslateFirmwareCode(result.Code);
		LatestResultText = translated;
		StatusMessage = translated;
		AddLog("system", translated, !result.IsSuccess);
	}

	private void UpdateConnectionState()
	{
		IsConnected = _bluetoothService.IsConnected;
		OnPropertyChanged(nameof(AdapterStateText));
		OnPropertyChanged(nameof(BleConnectionHintText));
		OnPropertyChanged(nameof(SelectedDeviceSummary));
	}

	private void HandleAdapterStateChanged(object? sender, BluetoothAdapterStateChangedEventArgs e)
	{
		MainThread.BeginInvokeOnMainThread(() =>
		{
			OnPropertyChanged(nameof(AdapterStateText));
			AddLog("system", $"Adapter BLE: {MapAdapterState(e.CurrentState)}.", false);
		});
	}

	private void HandleConnectionChanged(object? sender, BluetoothConnectionChangedEventArgs e)
	{
		MainThread.BeginInvokeOnMainThread(async () =>
		{
			UpdateConnectionState();
			StatusMessage = e.Message;
			AddLog("system", e.Message, e.State == BluetoothConnectionState.ConnectionLost);

			if (e.State == BluetoothConnectionState.Connected)
			{
				await RefreshStatusAsync();
			}
		});
	}

	private void ApplyLogFilter(bool criticalOnly)
	{
		_showCriticalLogsOnly = criticalOnly;
		VisibleLogs.Clear();

		var logs = _allLogs
			.Where(entry => !criticalOnly || entry.IsCritical)
			.OrderByDescending(entry => entry.Timestamp)
			.ToArray();

		foreach (var entry in logs)
		{
			VisibleLogs.Add(entry);
		}
	}

	private void ClearLogsView()
	{
		_allLogs.Clear();
		VisibleLogs.Clear();
		AddLog("system", "Widok logow wyczyszczony.", false);
	}

	private void AddLog(string category, string message, bool critical)
	{
		var entry = new ActivityLogEntry(DateTimeOffset.Now, category, message, critical);
		_allLogs.Add(entry);
		ApplyLogFilter(_showCriticalLogsOnly);
	}

	private void HandleException(string contextMessage, Exception exception, bool critical)
	{
		var detail = string.IsNullOrWhiteSpace(exception.Message) ? exception.GetType().Name : exception.Message;
		var message = $"{contextMessage} {detail}";
		StatusMessage = message;
		LatestResultText = message;
		OtaStatusText = message;
		AddLog("system", message, critical);
	}

	private static string TranslateFirmwareCode(string code)
	{
		return code switch
		{
			"settings_saved" => "Harmonogramy zapisane w kontrolerze.",
			"feed_now" => "Karmienie uruchomione.",
			"clear_logs" => "Krytyczne logi usuniete.",
			"invalid_light_mode" => "Niepoprawny tryb oswietlenia.",
			"invalid_aeration_mode" => "Niepoprawny tryb napowietrzania.",
			"invalid_filter_mode" => "Niepoprawny tryb filtra.",
			"invalid_heater_mode" => "Niepoprawny tryb grzalki.",
			"light_time_requires_schedule" => "Godziny oswietlenia sa dostepne tylko w trybie Harmonogram.",
			"aeration_time_requires_schedule" => "Godziny napowietrzania sa dostepne tylko w trybie Harmonogram.",
			"filter_time_requires_schedule" => "Godziny filtra sa dostepne tylko w trybie Harmonogram.",
			"invalid_light_start" => "Niepoprawna godzina startu oswietlenia.",
			"invalid_light_end" => "Niepoprawna godzina zakonczenia oswietlenia.",
			"invalid_aeration_start" => "Niepoprawna godzina startu napowietrzania.",
			"invalid_aeration_end" => "Niepoprawna godzina zakonczenia napowietrzania.",
			"invalid_filter_start" => "Niepoprawna godzina startu filtra.",
			"invalid_filter_end" => "Niepoprawna godzina zakonczenia filtra.",
			"invalid_target_temp" => "Prog grzalki musi miescic sie w zakresie 18-30 C.",
			"invalid_hysteresis" => "Histereza musi miescic sie w zakresie 0.1-5.0 C.",
			"invalid_feed_time" => "Karmienie wymaga godziny w kroku 5 minut.",
			"invalid_feed_mode" => "Niepoprawny tryb karmienia.",
			"invalid_payload" => "Sterownik odrzucil payload BLE.",
			"save_failed" => "Sterownik nie zapisal konfiguracji.",
			_ => string.IsNullOrWhiteSpace(code) ? "Sterownik zwrocil nieznany blad." : code
		};
	}

	private static string MapAdapterState(BleAdapterStatus status)
	{
		return status switch
		{
			BleAdapterStatus.On => "wlaczony",
			BleAdapterStatus.Off => "wylaczony",
			BleAdapterStatus.TurningOn => "uruchamianie",
			BleAdapterStatus.TurningOff => "wylaczanie",
			BleAdapterStatus.Unavailable => "niedostepny",
			_ => "nieznany"
		};
	}

	private static int ServoPercentToAngle(int percent)
	{
		return Math.Clamp((int)Math.Round(90d - ((percent / 100d) * 90d)), 0, 90);
	}

	private static Color ResolveBatteryColor(int percent)
	{
		return percent switch
		{
			>= 65 => Color.FromArgb("#34D399"),
			>= 35 => Color.FromArgb("#FACC15"),
			_ => Color.FromArgb("#FB7185")
		};
	}

	private static string AppendMessage(string existing, string extra)
	{
		if (string.IsNullOrWhiteSpace(existing))
		{
			return extra;
		}

		return $"{existing} {extra}";
	}

	private double ResolveDefaultThreshold()
	{
		return CurrentDeviceInfo.ValidationProfile.Temperature.Min > 0
			? CurrentDeviceInfo.ValidationProfile.Temperature.Min
			: AquariumValidationProfile.Default.Temperature.Min;
	}

	private double ResolveDefaultHysteresis()
	{
		return CurrentDeviceInfo.ValidationProfile.Hysteresis.Min > 0
			? CurrentDeviceInfo.ValidationProfile.Hysteresis.Min
			: AquariumValidationProfile.Default.Hysteresis.Min;
	}

	private void SetScheduleSelection<T>(ref SelectionOption<T>? field, SelectionOption<T>? value, [System.Runtime.CompilerServices.CallerMemberName] string? propertyName = null)
	{
		if (SetProperty(ref field, value, propertyName))
		{
			ValidateSchedules();
		}
	}

	private static IReadOnlyList<SelectionOption<int>> BuildMinuteOptions(int step)
	{
		var safeStep = step <= 0 ? 5 : step;
		return Enumerable.Range(0, (60 / safeStep) + 1)
			.Select(index => index * safeStep)
			.Where(minute => minute < 60)
			.Select(minute => new SelectionOption<int>(minute, minute.ToString("00", CultureInfo.InvariantCulture)))
			.ToArray();
	}

	private static IReadOnlyList<SelectionOption<int?>> BuildHeaterTargetOptions(AquariumNumericRule rule)
	{
		var values = new List<SelectionOption<int?>>();
		if (rule.SupportsOff)
		{
			values.Add(new SelectionOption<int?>(null, "OFF", "Grzalka zawsze wylaczona."));
		}

		var min = (int)Math.Round(rule.Min);
		var max = (int)Math.Round(rule.Max);
		for (var temp = min; temp <= max; temp++)
		{
			values.Add(new SelectionOption<int?>(temp, $"{temp} C", "Maksymalna temperatura pracy grzalki."));
		}

		return values;
	}

	private static IReadOnlyList<SelectionOption<double>> BuildHysteresisOptions(AquariumNumericRule rule)
	{
		var result = new List<SelectionOption<double>>();
		var current = rule.Min <= 0 ? 0.1 : rule.Min;
		var step = rule.Step <= 0 ? 0.1 : rule.Step;

		while (current <= rule.Max + 0.001d)
		{
			var rounded = Math.Round(current, 1, MidpointRounding.AwayFromZero);
			result.Add(new SelectionOption<double>(rounded, $"{rounded:0.0} C"));
			current += step;
		}

		return result;
	}

	private static IReadOnlyList<SelectionOption<int>> BuildFeedModes(AquariumIntegerRule rule)
	{
		var result = new List<SelectionOption<int>>();
		for (var value = rule.Min; value <= rule.Max; value++)
		{
			var label = value switch
			{
				0 => "Wylaczone",
				1 => "Codziennie",
				2 => "Co 2 dni",
				3 => "Co 3 dni",
				_ => $"Tryb {value}"
			};

			result.Add(new SelectionOption<int>(value, label));
		}

		return result;
	}

	private static SelectionOption<T>? FindOption<T>(IEnumerable<SelectionOption<T>> options, T? value)
	{
		if (options is null)
		{
			return default;
		}

		foreach (var option in options)
		{
			if (typeof(T) == typeof(double) && value is not null)
			{
				var optionValue = Convert.ToDouble(option.Value, CultureInfo.InvariantCulture);
				var targetValue = Convert.ToDouble(value, CultureInfo.InvariantCulture);
				if (Math.Abs(optionValue - targetValue) <= 0.051d)
				{
					return option;
				}

				continue;
			}

			if (EqualityComparer<T>.Default.Equals(option.Value, value!))
			{
				return option;
			}
		}

		return default;
	}
}
