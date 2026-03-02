$ErrorActionPreference = "Stop"

$code = @"
using System;
using System.Runtime.InteropServices;

public class CredReadOnly {
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
  public struct CREDENTIAL {
    public UInt32 Flags;
    public UInt32 Type;
    public string TargetName;
    public string Comment;
    public System.Runtime.InteropServices.ComTypes.FILETIME LastWritten;
    public UInt32 CredentialBlobSize;
    public IntPtr CredentialBlob;
    public UInt32 Persist;
    public UInt32 AttributeCount;
    public IntPtr Attributes;
    public string TargetAlias;
    public string UserName;
  }

  [DllImport("advapi32", EntryPoint="CredReadW", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern bool CredRead(string target, uint type, int reservedFlag, out IntPtr credentialPtr);

  [DllImport("advapi32", SetLastError=true)]
  public static extern void CredFree([In] IntPtr cred);
}
"@

Add-Type -TypeDefinition $code

# Type 1 = CRED_TYPE_GENERIC
$targets = @(
  "figma|d39d3b6252bc1ac5.Codex MCP Credentials",
  "LegacyGeneric:target=figma|d39d3b6252bc1ac5.Codex MCP Credentials"
)

$cred = $null
foreach ($target in $targets) {
  $ptr = [IntPtr]::Zero
  if ([CredReadOnly]::CredRead($target, 1, 0, [ref]$ptr)) {
    $cred = [Runtime.InteropServices.Marshal]::PtrToStructure($ptr, [Type][CredReadOnly+CREDENTIAL])
    break
  }
}

if ($null -eq $cred) {
  throw "Figma MCP credential not found. Run: codex mcp login figma"
}

$size = [int]$cred.CredentialBlobSize
$bytes = New-Object byte[] $size
[Runtime.InteropServices.Marshal]::Copy($cred.CredentialBlob, $bytes, 0, $size)
[CredReadOnly]::CredFree($ptr)

$json = [Text.Encoding]::Unicode.GetString($bytes).Trim([char]0)
if ($json.Length -gt 0 -and [int][char]$json[0] -eq 65279) {
  $json = $json.Substring(1)
}

$obj = $json | ConvertFrom-Json
$token = $obj.token_response.access_token

if ([string]::IsNullOrWhiteSpace($token)) {
  throw "No access_token found in credential blob."
}

Write-Output $token
