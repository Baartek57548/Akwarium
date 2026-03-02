param(
  [Parameter(Mandatory = $true)]
  [string]$CaptureId
)

$ErrorActionPreference = "Stop"

$token = powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/get-figma-token.ps1"
if ([string]::IsNullOrWhiteSpace($token)) {
  throw "Token retrieval failed."
}

$argsObj = @{
  captureId = $CaptureId
} | ConvertTo-Json -Compress

$env:FIGMA_MCP_TOKEN = $token
$env:MCP_TOOL_ARGS_JSON = $argsObj
try {
  node "$PSScriptRoot/mcp-call-tool.cjs" "generate_figma_design"
}
finally {
  Remove-Item Env:MCP_TOOL_ARGS_JSON -ErrorAction SilentlyContinue
  Remove-Item Env:FIGMA_MCP_TOKEN -ErrorAction SilentlyContinue
}
