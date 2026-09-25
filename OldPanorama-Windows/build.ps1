param([string]$Zig = 'zig')
$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$build = Join-Path $root 'build'
$dist = Join-Path $root 'dist'
$package = Join-Path $dist 'OldPanorama'
New-Item -ItemType Directory -Force -Path $build, $package | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $build 'zig-global-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $build 'zig-local-cache'
$source = Join-Path $root 'src/main.c'
$dll = Join-Path $build 'OldPanorama.dll'
$test = Join-Path $build 'patch-selftest.exe'
& $Zig cc -target x86_64-windows-gnu -O2 -shared $source -lbcrypt -o $dll
if ($LASTEXITCODE -ne 0) { throw 'DLL compilation failed' }
& $Zig cc -target x86_64-windows-gnu -O2 -DSELFTEST $source -lbcrypt -o $test
if ($LASTEXITCODE -ne 0) { throw 'Self-test compilation failed' }
& $test
if ($LASTEXITCODE -ne 0) { throw 'Patch self-tests failed' }
$log = Join-Path $build 'OldPanorama.log'
if (Test-Path -LiteralPath $log) { Remove-Item -LiteralPath $log }
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'test-load.ps1') -Dll $dll
if ($LASTEXITCODE -ne 0) { throw 'DLL loading test failed' }
Copy-Item -LiteralPath $dll -Destination $package
foreach ($name in @('manifest.json', 'README.md', 'LICENSE')) {
    Copy-Item -LiteralPath (Join-Path $root $name) -Destination $package
}
$manifest = Get-Content -LiteralPath (Join-Path $package 'manifest.json') -Raw | ConvertFrom-Json
$archive = Join-Path $dist ('OldPanorama-Windows-' + $manifest.version + '.zip')
Compress-Archive -LiteralPath $package -DestinationPath $archive -Force
Write-Host "Built and tested: $archive"
