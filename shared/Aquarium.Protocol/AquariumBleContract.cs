namespace Aquarium.Protocol;

public static class AquariumBleContract
{
  public const string DeviceName = "Akwarium_BLE";

  public static readonly Guid ServiceUuid = Guid.Parse("4fafc201-1fb5-459e-8bcc-c5c9c331914b");
  public static readonly Guid StatusCharacteristicUuid = Guid.Parse("beb5483e-36e1-4688-b7f5-ea07361b26a8");
  public static readonly Guid CommandCharacteristicUuid = Guid.Parse("828917c1-ea55-4d4a-a66e-fd202cea0645");
  public static readonly Guid SettingsCharacteristicUuid = Guid.Parse("d2912856-de63-11ed-b5ea-0242ac120002");
  public static readonly Guid ResultCharacteristicUuid = Guid.Parse("8e22cb9c-1728-45f9-8c50-2f7252f07379");
  public static readonly Guid DeviceInfoCharacteristicUuid = Guid.Parse("73d4b922-9d7d-4f5a-9f88-0871b07ec21b");
  public static readonly Guid OtaControlCharacteristicUuid = Guid.Parse("b5f6d0d0-0c6a-4cb0-a9b8-6b4e6cb6e550");
  public static readonly Guid OtaDataCharacteristicUuid = Guid.Parse("f2a4f5f5-89d0-4d3c-a4f7-e1db30c6ff0c");
}
