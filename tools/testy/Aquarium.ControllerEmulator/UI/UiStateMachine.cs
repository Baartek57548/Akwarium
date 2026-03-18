using Aquarium.ControllerEmulator.Device;

namespace Aquarium.ControllerEmulator.UI;

public enum UiScreen
{
  HomeScreen,
  Menu,
  Settings,
  System,
  SubMenu
}

public sealed class UiStateMachine
{
  private readonly UiModel _uiModel;
  private readonly IReadOnlyList<MenuItemDefinition> _rootMenuItems;
  private int _menuSelection;
  private int _settingsSelection;
  private int _systemSelection;
  private int _subMenuSelection;
  private string _activeStateName;
  private DateTime _lastInteractionUtc = DateTime.UtcNow;

  public UiStateMachine(UiModel uiModel)
  {
    _uiModel = uiModel;
    _rootMenuItems = uiModel.MenuItems.Count > 0 ? uiModel.MenuItems : BuildFallbackMenu();
    _activeStateName = DetectHomeState(uiModel.States);
  }

  public UiScreen CurrentScreen { get; private set; } = UiScreen.HomeScreen;
  public string LastAction { get; private set; } = "Ready";
  public string CurrentStateName => _activeStateName;

  public string CurrentMenuTitle => CurrentScreen switch
  {
    UiScreen.Menu => DetectMenuState(_uiModel.States),
    UiScreen.Settings => "SETTINGS",
    UiScreen.System => "SYSTEM",
    UiScreen.SubMenu => _activeStateName,
    _ => DetectHomeState(_uiModel.States)
  };

  public int CurrentSelection => CurrentScreen switch
  {
    UiScreen.Menu => _menuSelection,
    UiScreen.Settings => _settingsSelection,
    UiScreen.System => _systemSelection,
    UiScreen.SubMenu => _subMenuSelection,
    _ => 0
  };

  public void UpdateIdleTimeout(TimeSpan timeout)
  {
    if (CurrentScreen == UiScreen.HomeScreen)
    {
      return;
    }

    if (DateTime.UtcNow - _lastInteractionUtc < timeout)
    {
      return;
    }

    CurrentScreen = UiScreen.HomeScreen;
    _activeStateName = DetectHomeState(_uiModel.States);
    LastAction = "Idle timeout -> HOME";
  }

  public IReadOnlyList<string> GetCurrentEntries(DeviceState deviceState)
  {
    return CurrentScreen switch
    {
      UiScreen.Menu => _rootMenuItems.Select(item => item.Title).ToArray(),
      UiScreen.Settings =>
      [
        $"Target {deviceState.TargetTemperature:00.0}C",
        $"Heater {(deviceState.AutomaticHeaterControl ? "AUTO" : "MANUAL")}",
        "Clock +1 min",
        "Back"
      ],
      UiScreen.System =>
      [
        $"Light {(deviceState.LightOn ? "ON" : "OFF")}",
        $"Pump {(deviceState.PumpOn ? "ON" : "OFF")}",
        $"Heater {(deviceState.HeaterOn ? "ON" : "OFF")}",
        $"Feeder {(deviceState.FeederState == FeederState.Feeding ? "RUN" : "IDLE")}",
        "Back"
      ],
      UiScreen.SubMenu => GetSubMenuEntries(),
      _ => []
    };
  }

  public void Up()
  {
    Touch();
    switch (CurrentScreen)
    {
      case UiScreen.HomeScreen:
        return;
      case UiScreen.Menu:
        CurrentScreen = UiScreen.HomeScreen;
        _activeStateName = DetectHomeState(_uiModel.States);
        LastAction = "Back to HOME";
        return;
      case UiScreen.Settings:
      case UiScreen.System:
      case UiScreen.SubMenu:
        CurrentScreen = UiScreen.Menu;
        _activeStateName = DetectMenuState(_uiModel.States);
        LastAction = "Back to MENU";
        return;
    }
  }

  public void Down(DeviceState deviceState)
  {
    _ = deviceState;
    Touch();
    switch (CurrentScreen)
    {
      case UiScreen.HomeScreen:
        return;
      case UiScreen.Menu:
        _menuSelection = NextIndex(_menuSelection, _rootMenuItems.Count);
        LastAction = $"MENU item {_menuSelection + 1}";
        return;
      case UiScreen.Settings:
        _settingsSelection = NextIndex(_settingsSelection, 4);
        LastAction = $"SETTINGS item {_settingsSelection + 1}";
        return;
      case UiScreen.System:
        _systemSelection = NextIndex(_systemSelection, 5);
        LastAction = $"SYSTEM item {_systemSelection + 1}";
        return;
      case UiScreen.SubMenu:
      {
        var entries = GetSubMenuEntries();
        _subMenuSelection = NextIndex(_subMenuSelection, entries.Count);
        LastAction = $"{_activeStateName} item {_subMenuSelection + 1}";
        return;
      }
    }
  }

  public void Select(DeviceState deviceState)
  {
    Touch();
    switch (CurrentScreen)
    {
      case UiScreen.HomeScreen:
        CurrentScreen = UiScreen.Menu;
        _activeStateName = DetectMenuState(_uiModel.States);
        LastAction = "Open MENU";
        return;
      case UiScreen.Menu:
        SelectRootMenu(deviceState);
        return;
      case UiScreen.Settings:
        SelectSettings(deviceState);
        return;
      case UiScreen.System:
        SelectSystem(deviceState);
        return;
      case UiScreen.SubMenu:
        SelectSubMenu(deviceState);
        return;
    }
  }

  private void SelectRootMenu(DeviceState deviceState)
  {
    if (_rootMenuItems.Count == 0)
    {
      return;
    }

    var selected = _rootMenuItems[Math.Clamp(_menuSelection, 0, _rootMenuItems.Count - 1)];
    _activeStateName = selected.TargetState ?? selected.Title.ToUpperInvariant();

    var screen = ClassifyTarget(selected);
    CurrentScreen = screen;
    _subMenuSelection = 0;

    LastAction = screen switch
    {
      UiScreen.Settings => $"Open {_activeStateName}",
      UiScreen.System => $"Open {_activeStateName}",
      UiScreen.SubMenu => $"Open {_activeStateName}",
      UiScreen.HomeScreen => "Back to HOME",
      _ => "Open MENU"
    };

    if (_activeStateName.Contains("FEEDING", StringComparison.OrdinalIgnoreCase))
    {
      deviceState.StartFeeding();
      LastAction = "Feeding started";
    }
  }

  private void SelectSettings(DeviceState deviceState)
  {
    switch (_settingsSelection)
    {
      case 0:
        deviceState.IncreaseTargetTemperature();
        LastAction = $"Target {deviceState.TargetTemperature:00.0}C";
        break;
      case 1:
        deviceState.ToggleAutomaticHeaterControl();
        LastAction = deviceState.AutomaticHeaterControl ? "Heater AUTO" : "Heater MANUAL";
        break;
      case 2:
        deviceState.AdvanceClock(TimeSpan.FromMinutes(1));
        LastAction = "Clock +1 min";
        break;
      case 3:
        CurrentScreen = UiScreen.Menu;
        _activeStateName = DetectMenuState(_uiModel.States);
        LastAction = "Back to MENU";
        break;
    }
  }

  private void SelectSystem(DeviceState deviceState)
  {
    switch (_systemSelection)
    {
      case 0:
        deviceState.ToggleLight();
        LastAction = deviceState.LightOn ? "Light ON" : "Light OFF";
        break;
      case 1:
        deviceState.TogglePump();
        LastAction = deviceState.PumpOn ? "Pump ON" : "Pump OFF";
        break;
      case 2:
        deviceState.ToggleHeaterManual();
        LastAction = deviceState.HeaterOn ? "Heater ON" : "Heater OFF";
        break;
      case 3:
        deviceState.StartFeeding();
        LastAction = "Feeder started";
        break;
      case 4:
        CurrentScreen = UiScreen.Menu;
        _activeStateName = DetectMenuState(_uiModel.States);
        LastAction = "Back to MENU";
        break;
    }
  }

  private void SelectSubMenu(DeviceState deviceState)
  {
    var entries = GetSubMenuEntries();
    if (entries.Count == 0)
    {
      return;
    }

    var selected = entries[Math.Clamp(_subMenuSelection, 0, entries.Count - 1)];
    LastAction = selected;

    if (_activeStateName.Contains("FEEDING", StringComparison.OrdinalIgnoreCase))
    {
      deviceState.StartFeeding();
      LastAction = "Feeding started";
    }
  }

  private IReadOnlyList<string> GetSubMenuEntries()
  {
    var textElements = _uiModel.DisplayElements
      .Where(element => element.ScreenName.Equals(_activeStateName, StringComparison.OrdinalIgnoreCase))
      .OrderBy(element => element.Kind)
      .Select(element => element.Kind == DisplayElementKind.Text
        ? element.Value
        : $"{element.Kind}:{element.Value}")
      .Distinct(StringComparer.OrdinalIgnoreCase)
      .ToList();

    if (textElements.Count == 0)
    {
      textElements.Add($"State: {_activeStateName}");
      var screen = _uiModel.Screens.FirstOrDefault(item => item.Name.Equals(_activeStateName, StringComparison.OrdinalIgnoreCase));
      if (screen is not null)
      {
        textElements.Add($"Render:{screen.RenderFunction}");
      }
    }

    return textElements;
  }

  private UiScreen ClassifyTarget(MenuItemDefinition menuItem)
  {
    var title = menuItem.Title.ToLowerInvariant();
    var state = (menuItem.TargetState ?? string.Empty).ToUpperInvariant();

    if (state == "HOME" || title.Contains("back home", StringComparison.Ordinal))
    {
      return UiScreen.HomeScreen;
    }

    if (state == "MENU")
    {
      return UiScreen.Menu;
    }

    if (state.Contains("SCHEDULE", StringComparison.Ordinal) || state.Contains("SETTINGS", StringComparison.Ordinal) ||
        title.Contains("harmonogram", StringComparison.Ordinal) || title.Contains("data", StringComparison.Ordinal) ||
        title.Contains("czas", StringComparison.Ordinal))
    {
      return UiScreen.Settings;
    }

    if (title.Contains("test", StringComparison.Ordinal) || title.Contains("kalibr", StringComparison.Ordinal))
    {
      return UiScreen.System;
    }

    return UiScreen.SubMenu;
  }

  private static string DetectHomeState(IReadOnlyList<string> states)
  {
    return states.FirstOrDefault(state => state.Equals("HOME", StringComparison.OrdinalIgnoreCase)) ?? "HOME";
  }

  private static string DetectMenuState(IReadOnlyList<string> states)
  {
    return states.FirstOrDefault(state => state.Equals("MENU", StringComparison.OrdinalIgnoreCase)) ?? "MENU";
  }

  private static int NextIndex(int current, int count)
  {
    if (count <= 0)
    {
      return 0;
    }

    return (current + 1) % count;
  }

  private void Touch()
  {
    _lastInteractionUtc = DateTime.UtcNow;
  }

  private static IReadOnlyList<MenuItemDefinition> BuildFallbackMenu()
  {
    return
    [
      new MenuItemDefinition { Index = 0, Title = "Harmonogramy", TargetState = "SCHEDULE_LIGHT" },
      new MenuItemDefinition { Index = 1, Title = "Logi", TargetState = "LOGS" },
      new MenuItemDefinition { Index = 2, Title = "Data i Czas", TargetState = "SETTINGS_DATETIME" },
      new MenuItemDefinition { Index = 3, Title = "Test", TargetState = "TESTS" }
    ];
  }
}
