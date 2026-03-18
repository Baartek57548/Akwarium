namespace Aquarium.ControllerEmulator.UI;

public sealed class UiModel
{
  public required IReadOnlyList<string> States { get; init; }
  public required IReadOnlyList<ScreenDefinition> Screens { get; init; }
  public required IReadOnlyList<MenuItemDefinition> MenuItems { get; init; }
  public required IReadOnlyList<UiActionDefinition> Actions { get; init; }
  public required IReadOnlyList<DisplayElementDefinition> DisplayElements { get; init; }
  public required CodeStructureSummary CodeStructure { get; init; }
}

public sealed class ScreenDefinition
{
  public required string Name { get; init; }
  public required string RenderFunction { get; init; }
  public required string SourceFile { get; init; }
}

public sealed class MenuItemDefinition
{
  public required int Index { get; init; }
  public required string Title { get; init; }
  public string? TargetState { get; init; }
  public IReadOnlyList<MenuItemDefinition> Children { get; init; } = [];
}

public sealed class UiActionDefinition
{
  public required string Name { get; init; }
  public required string Trigger { get; init; }
  public required string InputBinding { get; init; }
  public required string SourceFile { get; init; }
}

public enum DisplayElementKind
{
  Text,
  Icon,
  StatusBar,
  MenuItem,
  Primitive,
  Animation
}

public sealed class DisplayElementDefinition
{
  public required string ScreenName { get; init; }
  public required DisplayElementKind Kind { get; init; }
  public required string Value { get; init; }
  public required string SourceFile { get; init; }
  public required string SourceFunction { get; init; }
}

public sealed class CodeStructureSummary
{
  public required IReadOnlyList<string> Classes { get; init; }
  public required IReadOnlyList<string> Enums { get; init; }
  public required IReadOnlyList<string> Functions { get; init; }
  public required IReadOnlyList<string> StateMachines { get; init; }
}
