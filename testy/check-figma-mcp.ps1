$ErrorActionPreference = "Stop"

$token = & "$PSScriptRoot/get-figma-token.ps1"
if ([string]::IsNullOrWhiteSpace($token)) {
  throw "Token retrieval failed."
}

$env:FIGMA_MCP_TOKEN = $token
try {
  node "$PSScriptRoot/mcp-list-tools.cjs"
}
finally {
  Remove-Item Env:FIGMA_MCP_TOKEN -ErrorAction SilentlyContinue
}
