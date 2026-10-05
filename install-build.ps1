param([string]$StagingName = 'MoeKuriTools.exe')
$ErrorActionPreference = 'Stop'
$target = Join-Path $PSScriptRoot 'MoeKuriTools.exe'
$staging = Join-Path $PSScriptRoot ('build\' + $StagingName)
if ([IO.Path]::GetFileName($StagingName) -ne $StagingName -or !(Test-Path -LiteralPath $staging -PathType Leaf)) { throw 'Invalid staging executable' }
$previous = Join-Path $PSScriptRoot 'MoeKuriTools.previous.exe'
if (Test-Path -LiteralPath $target) {
    if (Test-Path -LiteralPath $previous) {
        # Preserve a running or previously built copy under a unique name.
        $previous = Join-Path $PSScriptRoot ('MoeKuriTools.previous.' + [DateTime]::UtcNow.Ticks + '.exe')
    }
    Move-Item -LiteralPath $target -Destination $previous
}
Copy-Item -LiteralPath $staging -Destination $target
