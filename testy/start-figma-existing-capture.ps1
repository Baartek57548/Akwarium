param(
  [Parameter(Mandatory = $true)]
  [string]$FileOrUrl,
  [string]$NodeId
)

$ErrorActionPreference = "Stop"

function Get-FileKey([string]$InputValue) {
  $patterns = @(
    "figma\.com/design/([^/\?#]+)",
    "figma\.com/file/([^/\?#]+)",
    "figma\.com/integrations/claim/([^/\?#]+)"
  )

  foreach ($p in $patterns) {
    $m = [regex]::Match($InputValue, $p, [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
    if ($m.Success) {
      return $m.Groups[1].Value
    }
  }

  return $InputValue
}

$fileKey = Get-FileKey $FileOrUrl
if ([string]::IsNullOrWhiteSpace($fileKey)) {
  throw "Unable to resolve Figma file key from input."
}

$token = powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/get-figma-token.ps1"
if ([string]::IsNullOrWhiteSpace($token)) {
  throw "Token retrieval failed."
}

$argsObj = @{
  outputMode = "existingFile"
  fileKey = $fileKey
}

if (-not [string]::IsNullOrWhiteSpace($NodeId)) {
  $argsObj.nodeId = $NodeId
}

$env:FIGMA_MCP_TOKEN = $token
$env:MCP_TOOL_ARGS_JSON = ($argsObj | ConvertTo-Json -Compress)

try {
  node "$PSScriptRoot/mcp-call-tool.cjs" "generate_figma_design"
}
finally {
  Remove-Item Env:MCP_TOOL_ARGS_JSON -ErrorAction SilentlyContinue
  Remove-Item Env:FIGMA_MCP_TOKEN -ErrorAction SilentlyContinue
}
