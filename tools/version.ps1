param([Parameter(Mandatory)][string]$OutputDirectory)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$version = (Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if ($version -notmatch '^(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)$') {
    throw 'VERSION must contain MAJOR.MINOR.PATCH'
}
$parts = $version.Split('.')
foreach ($part in $parts) {
    if ([uint64]$part -gt 65535) { throw 'Version component exceeds Windows resource limits' }
}
$hash = git -C $root rev-parse --short=12 --verify --quiet HEAD
if ($LASTEXITCODE -ne 0) { $hash = 'uncommitted' }
$header = "#pragma once`n#define WEAVE_VERSION `"$version+$hash`"`n#define WEAVE_FILE_VERSION $($parts -join ','),0`n#define WEAVE_FILE_VERSION_STRING `"$version.0`"`n"
$path = Join-Path $OutputDirectory 'version.h'
if (!(Test-Path -LiteralPath $path) -or (Get-Content -LiteralPath $path -Raw) -ne $header) {
    [IO.File]::WriteAllText($path, $header, [Text.UTF8Encoding]::new($false))
}
