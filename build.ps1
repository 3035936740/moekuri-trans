param([switch]$Package, [switch]$SmokeTest)
$ErrorActionPreference = 'Stop'
Push-Location -LiteralPath $PSScriptRoot
try {
    & $env:ComSpec /d /c build.cmd
    if ($LASTEXITCODE -ne 0) { throw "MSVC build failed ($LASTEXITCODE)" }
    if ($SmokeTest -or $Package) {
        & python tests/ci_smoke.py
        if ($LASTEXITCODE -ne 0) { throw 'Build/dependency checks failed' }
    }
    if ($Package) { & (Join-Path $PSScriptRoot 'package-release.ps1') }
} finally { Pop-Location }
