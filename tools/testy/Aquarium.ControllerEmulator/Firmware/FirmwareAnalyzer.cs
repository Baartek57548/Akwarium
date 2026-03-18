using System.Text.RegularExpressions;
using Aquarium.ControllerEmulator.UI;

namespace Aquarium.ControllerEmulator.Firmware;

public sealed class FirmwareAnalysisResult
{
  public required string FirmwareSourceDirectory { get; init; }
  public required IReadOnlyList<string> SourceFiles { get; init; }
  public required CppCodeModel CodeModel { get; init; }
  public required UiModel UiModel { get; init; }
  public required DateTime AnalyzedAtUtc { get; init; }
}

public sealed class FirmwareAnalyzer
{
  private readonly CppParser _cppParser = new();

  private static readonly Regex MenuItemsArrayRegex = new(
    @"const\s+char\s+\*items\[\]\s*=\s*{(?<body>[\s\S]*?)};",
    RegexOptions.Compiled);

  private static readonly Regex QuotedStringRegex = new(
    "\"(?<value>(?:\\\\.|[^\"\\\\])*)\"",
    RegexOptions.Compiled);

  private static readonly Regex StateToRenderRegex = new(
    @"case\s+UiState::(?<state>[A-Z_]+)\s*:\s*(?<body>[\s\S]*?)(?:break\s*;|case\s+UiState::|default:)",
    RegexOptions.Compiled);

  private static readonly Regex RenderCallRegex = new(
    @"animation->(?<render>draw[A-Za-z0-9_]+)\s*\(",
    RegexOptions.Compiled);

  private static readonly Regex MenuSelectionBranchRegex = new(
    @"sel\s*==\s*(?<index>\d+)\s*\)\s*(?<body>\{[\s\S]*?\}|[^\n;]*;)",
    RegexOptions.Compiled);

  private static readonly Regex UiStateAssignmentRegex = new(
    @"uiState\s*=\s*UiState::(?<state>[A-Z_]+)",
    RegexOptions.Compiled);

  private static readonly Regex ButtonPinRegex = new(
    @"#define\s+(?<name>BUTTON_(?:UP|DOWN|SELECT)_PIN)\s+(?<pin>\d+)",
    RegexOptions.Compiled);

  private static readonly Regex DrawTextLiteralRegex = new(
    @"draw(?:Str|Text)\s*\([^,]+,[^,]+,\s*""(?<text>(?:\\\\.|[^""\\\\])*)""\s*\)",
    RegexOptions.Compiled);

  private static readonly Regex DrawIconRegex = new(
    @"drawXBMP\s*\([^,]+,[^,]+,[^,]+,[^,]+,\s*(?<icon>[A-Za-z0-9_]+)\s*\)",
    RegexOptions.Compiled);

  public FirmwareAnalysisResult Analyze(string repositoryRoot)
  {
    var sourceDirectory = Path.Combine(repositoryRoot, "firmware", "src");
    var sourceFiles = LoadSourceFiles(sourceDirectory);
    var codeModel = _cppParser.Parse(sourceFiles);
    var uiModel = BuildUiModel(sourceFiles, codeModel);

    return new FirmwareAnalysisResult
    {
      FirmwareSourceDirectory = sourceDirectory,
      SourceFiles = sourceFiles.Select(file => file.RelativePath).ToArray(),
      CodeModel = codeModel,
      UiModel = uiModel,
      AnalyzedAtUtc = DateTime.UtcNow
    };
  }

  private static IReadOnlyList<FirmwareSourceFile> LoadSourceFiles(string sourceDirectory)
  {
    if (!Directory.Exists(sourceDirectory))
    {
      return [];
    }

    var paths = Directory
      .EnumerateFiles(sourceDirectory, "*.*", SearchOption.AllDirectories)
      .Where(path =>
        path.EndsWith(".cpp", StringComparison.OrdinalIgnoreCase) ||
        path.EndsWith(".h", StringComparison.OrdinalIgnoreCase) ||
        path.EndsWith(".ino", StringComparison.OrdinalIgnoreCase))
      .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
      .ToArray();

    return paths
      .Select(path => new FirmwareSourceFile
      {
        AbsolutePath = path,
        RelativePath = Path.GetRelativePath(sourceDirectory, path).Replace('\\', '/'),
        Content = File.ReadAllText(path)
      })
      .ToArray();
  }

  private UiModel BuildUiModel(IReadOnlyList<FirmwareSourceFile> sourceFiles, CppCodeModel codeModel)
  {
    var states = ExtractUiStates(codeModel);
    var screens = ExtractScreens(sourceFiles, codeModel, states);
    var menuItems = ExtractMenuItems(sourceFiles);
    var actions = ExtractActions(sourceFiles);
    var displayElements = ExtractDisplayElements(codeModel, screens, menuItems);
    var summary = BuildCodeSummary(codeModel);

    return new UiModel
    {
      States = states,
      Screens = screens,
      MenuItems = menuItems,
      Actions = actions,
      DisplayElements = displayElements,
      CodeStructure = summary
    };
  }

  private static IReadOnlyList<string> ExtractUiStates(CppCodeModel codeModel)
  {
    var ordered = new List<string>();

    foreach (var @enum in codeModel.Enums.Where(item => item.Name.Equals("UiState", StringComparison.OrdinalIgnoreCase)))
    {
      foreach (var value in @enum.Values)
      {
        if (!ordered.Contains(value, StringComparer.OrdinalIgnoreCase))
        {
          ordered.Add(value);
        }
      }
    }

    foreach (var machine in codeModel.StateMachines)
    {
      foreach (var @case in machine.Cases)
      {
        var value = @case.Replace("UiState::", string.Empty, StringComparison.OrdinalIgnoreCase).Trim();
        if (value.Length > 0 && !ordered.Contains(value, StringComparer.OrdinalIgnoreCase))
        {
          ordered.Add(value);
        }
      }

      foreach (var transition in machine.Transitions)
      {
        if (!ordered.Contains(transition, StringComparer.OrdinalIgnoreCase))
        {
          ordered.Add(transition);
        }
      }
    }

    return ordered;
  }

  private static IReadOnlyList<ScreenDefinition> ExtractScreens(
    IReadOnlyList<FirmwareSourceFile> sourceFiles,
    CppCodeModel codeModel,
    IReadOnlyList<string> states)
  {
    var screens = new List<ScreenDefinition>();
    var akwariumFile = sourceFiles.FirstOrDefault(file => file.RelativePath.EndsWith("AkwariumV4.ino", StringComparison.OrdinalIgnoreCase));

    if (akwariumFile is not null)
    {
      foreach (Match match in StateToRenderRegex.Matches(akwariumFile.Content))
      {
        var state = match.Groups["state"].Value.Trim();
        if (state.Length == 0)
        {
          continue;
        }

        var renderMatch = RenderCallRegex.Match(match.Groups["body"].Value);
        if (!renderMatch.Success)
        {
          continue;
        }

        var renderFunction = renderMatch.Groups["render"].Value.Trim();
        if (renderFunction.Length == 0)
        {
          continue;
        }

        if (screens.Any(existing => existing.Name.Equals(state, StringComparison.OrdinalIgnoreCase)))
        {
          continue;
        }

        screens.Add(new ScreenDefinition
        {
          Name = state,
          RenderFunction = renderFunction,
          SourceFile = akwariumFile.RelativePath
        });
      }
    }

    foreach (var function in codeModel.Functions.Where(fn =>
               fn.Name.Contains("draw", StringComparison.OrdinalIgnoreCase) ||
               fn.Name.Contains("render", StringComparison.OrdinalIgnoreCase)))
    {
      var functionName = function.Name.Contains("::", StringComparison.Ordinal)
        ? function.Name[(function.Name.LastIndexOf("::", StringComparison.Ordinal) + 2)..]
        : function.Name;

      if (!functionName.StartsWith("draw", StringComparison.OrdinalIgnoreCase))
      {
        continue;
      }

      var guessedState = GuessStateFromRenderFunction(functionName, states);
      if (screens.Any(existing => existing.Name.Equals(guessedState, StringComparison.OrdinalIgnoreCase)))
      {
        continue;
      }

      screens.Add(new ScreenDefinition
      {
        Name = guessedState,
        RenderFunction = functionName,
        SourceFile = function.SourceFile
      });
    }

    return screens;
  }

  private static IReadOnlyList<MenuItemDefinition> ExtractMenuItems(IReadOnlyList<FirmwareSourceFile> sourceFiles)
  {
    var animation = sourceFiles.FirstOrDefault(file => file.RelativePath.EndsWith("AquariumAnimation.cpp", StringComparison.OrdinalIgnoreCase));
    var akwarium = sourceFiles.FirstOrDefault(file => file.RelativePath.EndsWith("AkwariumV4.ino", StringComparison.OrdinalIgnoreCase));

    if (animation is null)
    {
      return [];
    }

    var labels = new List<string>();
    var menuMatch = MenuItemsArrayRegex.Match(animation.Content);
    if (menuMatch.Success)
    {
      foreach (Match text in QuotedStringRegex.Matches(menuMatch.Groups["body"].Value))
      {
        var label = text.Groups["value"].Value.Trim();
        if (label.Length > 0)
        {
          labels.Add(label);
        }
      }
    }

    var targets = akwarium is null
      ? new Dictionary<int, string>()
      : ExtractMenuTargets(akwarium.Content);

    var menu = new List<MenuItemDefinition>();
    for (var index = 0; index < labels.Count; index++)
    {
      menu.Add(new MenuItemDefinition
      {
        Index = index,
        Title = labels[index],
        TargetState = targets.TryGetValue(index, out var state) ? state : null
      });
    }

    return menu;
  }

  private static IReadOnlyList<UiActionDefinition> ExtractActions(IReadOnlyList<FirmwareSourceFile> sourceFiles)
  {
    var actions = new List<UiActionDefinition>();
    foreach (var file in sourceFiles)
    {
      foreach (Match match in ButtonPinRegex.Matches(file.Content))
      {
        var rawName = match.Groups["name"].Value;
        var pin = match.Groups["pin"].Value;
        var actionName = rawName.Replace("BUTTON_", string.Empty, StringComparison.OrdinalIgnoreCase).Replace("_PIN", string.Empty, StringComparison.OrdinalIgnoreCase);
        var inputBinding = actionName switch
        {
          "UP" => "ArrowUp",
          "DOWN" => "ArrowDown",
          "SELECT" => "Enter",
          _ => "Unknown"
        };

        if (actions.Any(existing => existing.Name.Equals(actionName, StringComparison.OrdinalIgnoreCase)))
        {
          continue;
        }

        actions.Add(new UiActionDefinition
        {
          Name = actionName,
          Trigger = $"{rawName}={pin}",
          InputBinding = inputBinding,
          SourceFile = file.RelativePath
        });
      }
    }

    if (!actions.Any(action => action.Name.Equals("UP", StringComparison.OrdinalIgnoreCase)))
    {
      actions.Add(new UiActionDefinition { Name = "UP", Trigger = "BUTTON_UP_PIN", InputBinding = "ArrowUp", SourceFile = "inferred" });
    }
    if (!actions.Any(action => action.Name.Equals("SELECT", StringComparison.OrdinalIgnoreCase)))
    {
      actions.Add(new UiActionDefinition { Name = "SELECT", Trigger = "BUTTON_SELECT_PIN", InputBinding = "Enter", SourceFile = "inferred" });
    }
    if (!actions.Any(action => action.Name.Equals("DOWN", StringComparison.OrdinalIgnoreCase)))
    {
      actions.Add(new UiActionDefinition { Name = "DOWN", Trigger = "BUTTON_DOWN_PIN", InputBinding = "ArrowDown", SourceFile = "inferred" });
    }

    return actions;
  }

  private static IReadOnlyList<DisplayElementDefinition> ExtractDisplayElements(
    CppCodeModel codeModel,
    IReadOnlyList<ScreenDefinition> screens,
    IReadOnlyList<MenuItemDefinition> menuItems)
  {
    var byRender = screens
      .GroupBy(screen => screen.RenderFunction, StringComparer.OrdinalIgnoreCase)
      .ToDictionary(group => group.Key, group => group.First(), StringComparer.OrdinalIgnoreCase);
    var elements = new List<DisplayElementDefinition>();

    foreach (var function in codeModel.Functions)
    {
      var functionName = function.Name.Contains("::", StringComparison.Ordinal)
        ? function.Name[(function.Name.LastIndexOf("::", StringComparison.Ordinal) + 2)..]
        : function.Name;

      if (!byRender.TryGetValue(functionName, out var screen))
      {
        continue;
      }

      foreach (Match text in DrawTextLiteralRegex.Matches(function.Body))
      {
        var value = text.Groups["text"].Value.Trim();
        if (value.Length == 0)
        {
          continue;
        }

        elements.Add(new DisplayElementDefinition
        {
          ScreenName = screen.Name,
          Kind = DisplayElementKind.Text,
          Value = value,
          SourceFile = function.SourceFile,
          SourceFunction = functionName
        });
      }

      foreach (Match icon in DrawIconRegex.Matches(function.Body))
      {
        var value = icon.Groups["icon"].Value.Trim();
        if (value.Length == 0)
        {
          continue;
        }

        elements.Add(new DisplayElementDefinition
        {
          ScreenName = screen.Name,
          Kind = DisplayElementKind.Icon,
          Value = value,
          SourceFile = function.SourceFile,
          SourceFunction = functionName
        });
      }

      var primitiveCalls = codeModel.DrawCalls
        .Where(call => call.SourceFile.Equals(function.SourceFile, StringComparison.OrdinalIgnoreCase) &&
                       call.Line >= function.Line &&
                       call.Line <= function.Line + CountLines(function.Body))
        .Select(call => call.CallName)
        .Distinct(StringComparer.OrdinalIgnoreCase)
        .ToArray();

      foreach (var primitive in primitiveCalls)
      {
        elements.Add(new DisplayElementDefinition
        {
          ScreenName = screen.Name,
          Kind = primitive.Contains("drawBox", StringComparison.OrdinalIgnoreCase) ||
                 primitive.Contains("drawFrame", StringComparison.OrdinalIgnoreCase)
            ? DisplayElementKind.StatusBar
            : DisplayElementKind.Primitive,
          Value = primitive,
          SourceFile = function.SourceFile,
          SourceFunction = functionName
        });
      }
    }

    foreach (var menuItem in menuItems)
    {
      elements.Add(new DisplayElementDefinition
      {
        ScreenName = "MENU",
        Kind = DisplayElementKind.MenuItem,
        Value = menuItem.Title,
        SourceFile = "AquariumAnimation.cpp",
        SourceFunction = "drawMenu"
      });
    }

    return elements
      .DistinctBy(element => $"{element.ScreenName}|{element.Kind}|{element.Value}|{element.SourceFunction}")
      .ToArray();
  }

  private static CodeStructureSummary BuildCodeSummary(CppCodeModel codeModel)
  {
    return new CodeStructureSummary
    {
      Classes = codeModel.Classes.Select(item => item.Name).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(name => name).ToArray(),
      Enums = codeModel.Enums.Select(item => item.Name).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(name => name).ToArray(),
      Functions = codeModel.Functions.Select(item => item.Name).Distinct(StringComparer.OrdinalIgnoreCase).OrderBy(name => name).ToArray(),
      StateMachines = codeModel.StateMachines
        .Select(machine => $"{machine.Expression} ({machine.SourceFile}:{machine.Line})")
        .Distinct(StringComparer.OrdinalIgnoreCase)
        .ToArray()
    };
  }

  private static Dictionary<int, string> ExtractMenuTargets(string akwariumContent)
  {
    var result = new Dictionary<int, string>();
    var menuCaseBody = ExtractMenuCaseBody(akwariumContent);
    if (menuCaseBody.Length == 0)
    {
      return result;
    }

    foreach (Match branch in MenuSelectionBranchRegex.Matches(menuCaseBody))
    {
      if (!int.TryParse(branch.Groups["index"].Value, out var index))
      {
        continue;
      }

      var body = branch.Groups["body"].Value;
      var stateMatch = UiStateAssignmentRegex.Match(body);
      if (stateMatch.Success)
      {
        result[index] = stateMatch.Groups["state"].Value.Trim();
        continue;
      }

      if (body.Contains("runFeederCalibration", StringComparison.Ordinal))
      {
        result[index] = "FEEDING";
      }
      else if (body.Contains("startAP", StringComparison.Ordinal))
      {
        result[index] = "ACCESS_POINT";
      }
      else if (body.Contains("BleManager::start", StringComparison.Ordinal))
      {
        result[index] = "BLUETOOTH";
      }
      else
      {
        result[index] = "MENU";
      }
    }

    return result;
  }

  private static string ExtractMenuCaseBody(string content)
  {
    var marker = "case UiState::MENU:";
    var start = content.IndexOf(marker, StringComparison.Ordinal);
    if (start < 0)
    {
      return string.Empty;
    }

    var nextCase = content.IndexOf("case UiState::", start + marker.Length, StringComparison.Ordinal);
    if (nextCase < 0)
    {
      nextCase = content.Length;
    }

    return content.Substring(start, nextCase - start);
  }

  private static string GuessStateFromRenderFunction(string functionName, IReadOnlyList<string> states)
  {
    var withoutPrefix = functionName.StartsWith("draw", StringComparison.OrdinalIgnoreCase)
      ? functionName[4..]
      : functionName;
    var normalized = withoutPrefix.Replace("_", string.Empty, StringComparison.Ordinal).ToUpperInvariant();

    foreach (var state in states)
    {
      var stateNormalized = state.Replace("_", string.Empty, StringComparison.Ordinal).ToUpperInvariant();
      if (stateNormalized.Contains(normalized, StringComparison.Ordinal) || normalized.Contains(stateNormalized, StringComparison.Ordinal))
      {
        return state;
      }
    }

    return withoutPrefix.ToUpperInvariant();
  }

  private static int CountLines(string content)
  {
    if (content.Length == 0)
    {
      return 0;
    }

    var lines = 1;
    foreach (var character in content)
    {
      if (character == '\n')
      {
        lines++;
      }
    }

    return lines;
  }
}
