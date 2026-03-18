namespace Aquarium.ControllerEmulator.Device;

public enum FeederState
{
  Idle,
  Feeding
}

public sealed class DeviceState
{
  private readonly Random _random = new();
  private const double AmbientTemperature = 23.5;
  private double _batteryAccumulatorSeconds;
  private double _feedingSecondsRemaining;

  public DeviceState()
  {
    Temperature = 24.3;
    TargetTemperature = 24.0;
    LightOn = true;
    PumpOn = true;
    HeaterOn = false;
    FeederState = FeederState.Idle;
    CurrentTime = DateTime.Now;
    BatteryPercent = 92;
    AutomaticHeaterControl = true;
  }

  public double Temperature { get; private set; }
  public double TargetTemperature { get; private set; }
  public bool LightOn { get; private set; }
  public bool PumpOn { get; private set; }
  public bool HeaterOn { get; private set; }
  public FeederState FeederState { get; private set; }
  public DateTime CurrentTime { get; private set; }
  public int BatteryPercent { get; private set; }
  public bool AutomaticHeaterControl { get; private set; }

  public void Update(TimeSpan elapsed)
  {
    if (elapsed <= TimeSpan.Zero)
    {
      return;
    }

    CurrentTime = CurrentTime.Add(elapsed);
    UpdateFeeder(elapsed);
    UpdateTemperature(elapsed);
    UpdateBattery(elapsed);
  }

  public void ToggleLight()
  {
    LightOn = !LightOn;
  }

  public void TogglePump()
  {
    PumpOn = !PumpOn;
  }

  public void ToggleHeaterManual()
  {
    AutomaticHeaterControl = false;
    HeaterOn = !HeaterOn;
  }

  public void ToggleAutomaticHeaterControl()
  {
    AutomaticHeaterControl = !AutomaticHeaterControl;
  }

  public void IncreaseTargetTemperature()
  {
    TargetTemperature += 0.5;
    if (TargetTemperature > 30.0)
    {
      TargetTemperature = 18.0;
    }
  }

  public void AdvanceClock(TimeSpan delta)
  {
    CurrentTime = CurrentTime.Add(delta);
  }

  public void StartFeeding()
  {
    FeederState = FeederState.Feeding;
    _feedingSecondsRemaining = 4.0;
  }

  private void UpdateFeeder(TimeSpan elapsed)
  {
    if (FeederState != FeederState.Feeding)
    {
      return;
    }

    _feedingSecondsRemaining -= elapsed.TotalSeconds;
    if (_feedingSecondsRemaining <= 0)
    {
      _feedingSecondsRemaining = 0;
      FeederState = FeederState.Idle;
    }
  }

  private void UpdateTemperature(TimeSpan elapsed)
  {
    if (AutomaticHeaterControl)
    {
      if (Temperature <= TargetTemperature - 0.3)
      {
        HeaterOn = true;
      }
      else if (Temperature >= TargetTemperature + 0.2)
      {
        HeaterOn = false;
      }
    }

    var heaterEffect = HeaterOn ? 0.020 * elapsed.TotalSeconds : -0.010 * elapsed.TotalSeconds;
    var ambientPull = (AmbientTemperature - Temperature) * 0.010 * elapsed.TotalSeconds;
    var noise = (_random.NextDouble() - 0.5) * 0.030;
    Temperature = Math.Clamp(Temperature + heaterEffect + ambientPull + noise, 18.0, 32.0);
  }

  private void UpdateBattery(TimeSpan elapsed)
  {
    _batteryAccumulatorSeconds += elapsed.TotalSeconds;
    if (_batteryAccumulatorSeconds < 45.0)
    {
      return;
    }

    _batteryAccumulatorSeconds = 0;
    if (BatteryPercent <= 10)
    {
      return;
    }

    var load = 0;
    if (LightOn)
    {
      load++;
    }
    if (PumpOn)
    {
      load++;
    }
    if (HeaterOn)
    {
      load++;
    }
    if (FeederState == FeederState.Feeding)
    {
      load++;
    }

    if (load > 0 && _random.NextDouble() > 0.35)
    {
      BatteryPercent = Math.Max(10, BatteryPercent - 1);
    }
  }
}
