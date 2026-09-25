param([Parameter(Mandatory=$true)][string]$Dll)
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class NativeLoadTest {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
    public static extern IntPtr LoadLibraryW(string path);
}
'@
$module = [NativeLoadTest]::LoadLibraryW($Dll)
if ($module -eq [IntPtr]::Zero) { throw 'Windows could not load the DLL' }
$log = Join-Path (Split-Path -Parent $Dll) 'OldPanorama.log'
for ($attempt=0; $attempt -lt 100; $attempt++) {
    if (Test-Path -LiteralPath $log) {
        $message = Get-Content -LiteralPath $log -Raw
        if ($message -like 'NOT APPLIED: unsupported executable*') {
            Write-Host 'PASS: Windows DLL loads and rejects unsupported host.'
            exit 0
        }
    }
    Start-Sleep -Milliseconds 100
}
throw 'Expected rejection log was not produced'
