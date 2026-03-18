using System.Text.RegularExpressions;

namespace Aquarium.ControllerEmulator.Firmware;

public sealed class FirmwareSourceFile
{
  public required string RelativePath { get; init; }
  public required string AbsolutePath { get; init; }
  public required string Content { get; init; }
}

public sealed class CppCodeModel
{
  public required IReadOnlyList<CppFileModel> Files { get; init; }
  public required IReadOnlyList<CppClassModel> Classes { get; init; }
  public required IReadOnlyList<CppEnumModel> Enums { get; init; }
  public required IReadOnlyList<CppFunctionModel> Functions { get; init; }
  public required IReadOnlyList<CppStateMachineModel> StateMachines { get; init; }
  public required IReadOnlyList<CppStringLiteralModel> StringLiterals { get; init; }
  public required IReadOnlyList<CppDrawCallModel> DrawCalls { get; init; }
}

public sealed class CppFileModel
{
  public required string RelativePath { get; init; }
  public required int LineCount { get; init; }
}

public sealed class CppClassModel
{
  public required string Name { get; init; }
  public required string Kind { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppEnumModel
{
  public required string Name { get; init; }
  public required IReadOnlyList<string> Values { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppFunctionModel
{
  public required string Name { get; init; }
  public required string ReturnType { get; init; }
  public required string Parameters { get; init; }
  public required string Signature { get; init; }
  public required string Body { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppStateMachineModel
{
  public required string Expression { get; init; }
  public required IReadOnlyList<string> Cases { get; init; }
  public required IReadOnlyList<string> Transitions { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppStringLiteralModel
{
  public required string Value { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppDrawCallModel
{
  public required string CallName { get; init; }
  public required string SourceFile { get; init; }
  public required int Line { get; init; }
}

public sealed class CppParser
{
  private static readonly Regex ClassRegex = new(
    @"\b(?<kind>class|struct)\s+(?<name>[A-Za-z_][A-Za-z0-9_]*)\b",
    RegexOptions.Compiled);

  private static readonly Regex EnumRegex = new(
    @"enum(?:\s+class)?\s+(?<name>[A-Za-z_][A-Za-z0-9_]*)\s*\{(?<body>[\s\S]*?)\};",
    RegexOptions.Compiled);

  private static readonly Regex FunctionRegex = new(
    @"(?m)^(?<ret>[~A-Za-z_][A-Za-z0-9_:<>\s\*&,\[\]]*?)\s+(?<name>[A-Za-z_][A-Za-z0-9_:]*)\s*\((?<params>[^;{}()]*)\)\s*(?<suffix>(?:const|noexcept|override|final|\s)*)\{",
    RegexOptions.Compiled);

  private static readonly Regex SwitchRegex = new(
    @"switch\s*\((?<expr>[^)]+)\)\s*\{",
    RegexOptions.Compiled);

  private static readonly Regex CaseRegex = new(
    @"case\s+(?<label>[^:]+)\s*:",
    RegexOptions.Compiled);

  private static readonly Regex UiTransitionRegex = new(
    @"uiState\s*=\s*UiState::(?<state>[A-Z_]+)",
    RegexOptions.Compiled);

  private static readonly Regex StringLiteralRegex = new(
    "\"(?<value>(?:\\\\.|[^\"\\\\])*)\"",
    RegexOptions.Compiled);

  private static readonly Regex DrawCallRegex = new(
    @"(?<name>(?:display->|animation->|u8g2\.)?(?:draw|render)[A-Za-z0-9_]+)\s*\(",
    RegexOptions.Compiled);

  public CppCodeModel Parse(IReadOnlyList<FirmwareSourceFile> files)
  {
    var fileModels = new List<CppFileModel>();
    var classes = new List<CppClassModel>();
    var enums = new List<CppEnumModel>();
    var functions = new List<CppFunctionModel>();
    var stateMachines = new List<CppStateMachineModel>();
    var literals = new List<CppStringLiteralModel>();
    var drawCalls = new List<CppDrawCallModel>();

    foreach (var file in files)
    {
      fileModels.Add(new CppFileModel
      {
        RelativePath = file.RelativePath,
        LineCount = CountLines(file.Content)
      });

      classes.AddRange(ParseClasses(file));
      enums.AddRange(ParseEnums(file));
      functions.AddRange(ParseFunctions(file));
      stateMachines.AddRange(ParseStateMachines(file));
      literals.AddRange(ParseStringLiterals(file));
      drawCalls.AddRange(ParseDrawCalls(file));
    }

    return new CppCodeModel
    {
      Files = fileModels,
      Classes = classes,
      Enums = enums,
      Functions = functions,
      StateMachines = stateMachines,
      StringLiterals = literals,
      DrawCalls = drawCalls
    };
  }

  private static IEnumerable<CppClassModel> ParseClasses(FirmwareSourceFile file)
  {
    foreach (Match match in ClassRegex.Matches(file.Content))
    {
      var name = match.Groups["name"].Value;
      if (name.Length == 0)
      {
        continue;
      }

      yield return new CppClassModel
      {
        Name = name,
        Kind = match.Groups["kind"].Value,
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IEnumerable<CppEnumModel> ParseEnums(FirmwareSourceFile file)
  {
    foreach (Match match in EnumRegex.Matches(file.Content))
    {
      var name = match.Groups["name"].Value;
      if (name.Length == 0)
      {
        continue;
      }

      var values = ExtractEnumValues(match.Groups["body"].Value);
      yield return new CppEnumModel
      {
        Name = name,
        Values = values,
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IEnumerable<CppFunctionModel> ParseFunctions(FirmwareSourceFile file)
  {
    foreach (Match match in FunctionRegex.Matches(file.Content))
    {
      var functionName = match.Groups["name"].Value.Trim();
      var returnType = match.Groups["ret"].Value.Trim();
      if (!IsLikelyFunction(returnType, functionName))
      {
        continue;
      }

      var bodyStart = file.Content.IndexOf('{', match.Index + match.Length - 1);
      if (bodyStart < 0)
      {
        continue;
      }

      var bodyEnd = FindMatchingBrace(file.Content, bodyStart);
      if (bodyEnd <= bodyStart)
      {
        continue;
      }

      yield return new CppFunctionModel
      {
        Name = functionName,
        ReturnType = returnType,
        Parameters = match.Groups["params"].Value.Trim(),
        Signature = file.Content.Substring(match.Index, bodyStart - match.Index + 1).Trim(),
        Body = file.Content.Substring(bodyStart + 1, bodyEnd - bodyStart - 1),
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IEnumerable<CppStateMachineModel> ParseStateMachines(FirmwareSourceFile file)
  {
    foreach (Match match in SwitchRegex.Matches(file.Content))
    {
      var openBrace = file.Content.IndexOf('{', match.Index + match.Length - 1);
      if (openBrace < 0)
      {
        continue;
      }

      var closeBrace = FindMatchingBrace(file.Content, openBrace);
      if (closeBrace <= openBrace)
      {
        continue;
      }

      var block = file.Content.Substring(openBrace + 1, closeBrace - openBrace - 1);
      var cases = CaseRegex.Matches(block)
        .Select(m => m.Groups["label"].Value.Trim())
        .Where(label => !string.IsNullOrWhiteSpace(label))
        .Distinct(StringComparer.OrdinalIgnoreCase)
        .ToArray();

      if (cases.Length == 0)
      {
        continue;
      }

      var transitions = UiTransitionRegex.Matches(block)
        .Select(m => m.Groups["state"].Value.Trim())
        .Where(value => value.Length > 0)
        .Distinct(StringComparer.OrdinalIgnoreCase)
        .ToArray();

      yield return new CppStateMachineModel
      {
        Expression = match.Groups["expr"].Value.Trim(),
        Cases = cases,
        Transitions = transitions,
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IEnumerable<CppStringLiteralModel> ParseStringLiterals(FirmwareSourceFile file)
  {
    foreach (Match match in StringLiteralRegex.Matches(file.Content))
    {
      var raw = match.Groups["value"].Value;
      if (string.IsNullOrWhiteSpace(raw))
      {
        continue;
      }

      yield return new CppStringLiteralModel
      {
        Value = raw,
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IEnumerable<CppDrawCallModel> ParseDrawCalls(FirmwareSourceFile file)
  {
    foreach (Match match in DrawCallRegex.Matches(file.Content))
    {
      var call = match.Groups["name"].Value.Trim();
      if (call.Length == 0)
      {
        continue;
      }

      yield return new CppDrawCallModel
      {
        CallName = call,
        SourceFile = file.RelativePath,
        Line = GetLine(file.Content, match.Index)
      };
    }
  }

  private static IReadOnlyList<string> ExtractEnumValues(string body)
  {
    var values = new List<string>();
    foreach (var token in body.Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries))
    {
      var candidate = token;
      var commentIndex = candidate.IndexOf("//", StringComparison.Ordinal);
      if (commentIndex >= 0)
      {
        candidate = candidate[..commentIndex];
      }

      var assignmentIndex = candidate.IndexOf('=', StringComparison.Ordinal);
      if (assignmentIndex >= 0)
      {
        candidate = candidate[..assignmentIndex];
      }

      candidate = candidate.Trim();
      if (candidate.Length == 0)
      {
        continue;
      }

      var parts = candidate.Split([' ', '\t', '\r', '\n'], StringSplitOptions.RemoveEmptyEntries);
      if (parts.Length == 0)
      {
        continue;
      }

      var value = parts[^1].Trim();
      if (!values.Contains(value, StringComparer.OrdinalIgnoreCase))
      {
        values.Add(value);
      }
    }

    return values;
  }

  private static bool IsLikelyFunction(string returnType, string name)
  {
    if (name.Contains("operator", StringComparison.Ordinal))
    {
      return false;
    }

    var lowered = returnType.Trim().ToLowerInvariant();
    if (lowered.StartsWith("if ") || lowered.StartsWith("for ") || lowered.StartsWith("while ") || lowered.StartsWith("switch "))
    {
      return false;
    }

    if (name is "if" or "for" or "while" or "switch" or "catch")
    {
      return false;
    }

    return true;
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

  private static int GetLine(string content, int index)
  {
    var line = 1;
    for (var i = 0; i < index && i < content.Length; i++)
    {
      if (content[i] == '\n')
      {
        line++;
      }
    }

    return line;
  }

  private static int FindMatchingBrace(string text, int openingBraceIndex)
  {
    var depth = 0;
    var inString = false;
    var inChar = false;
    var escape = false;

    for (var i = openingBraceIndex; i < text.Length; i++)
    {
      var character = text[i];

      if (escape)
      {
        escape = false;
        continue;
      }

      if (character == '\\')
      {
        escape = true;
        continue;
      }

      if (!inChar && character == '"')
      {
        inString = !inString;
        continue;
      }

      if (!inString && character == '\'')
      {
        inChar = !inChar;
        continue;
      }

      if (inString || inChar)
      {
        continue;
      }

      if (character == '{')
      {
        depth++;
      }
      else if (character == '}')
      {
        depth--;
        if (depth == 0)
        {
          return i;
        }
      }
    }

    return -1;
  }
}
