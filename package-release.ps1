$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Explicit allowlist: never recursively package the working directory.
$files = @('MoeKuriTools.exe', 'README.md', '1.10trans.txt', 'font.ttf', 'FONT-LICENSE-OFL.txt', 'FONT-NOTICES.txt')
$destination = Join-Path $PSScriptRoot 'dist'
[IO.Directory]::CreateDirectory($destination) | Out-Null
$zipPath = Join-Path $destination 'moekuri-trans-windows-x86.zip'
$temporary = Join-Path $destination ('release-' + [Guid]::NewGuid().ToString('N') + '.tmp')
$archive = [IO.Compression.ZipFile]::Open($temporary, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($name in $files) {
        $path = Join-Path $PSScriptRoot $name
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required release file missing: $name" }
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $path, $name, [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} catch {
    $archive.Dispose()
    Remove-Item -LiteralPath $temporary
    throw
} finally { $archive.Dispose() }
try {
    if ([IO.File]::Exists($zipPath)) {
        [IO.File]::Replace($temporary, $zipPath, [NullString]::Value)
    } else {
        [IO.File]::Move($temporary, $zipPath)
    }
} catch {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary }
    throw
}
$verify = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $names = @($verify.Entries | ForEach-Object { $_.FullName })
    if (@(Compare-Object ($files | Sort-Object) ($names | Sort-Object)).Count -ne 0) { throw 'Release contains unexpected files' }
} finally { $verify.Dispose() }
$fontPath = Join-Path $destination 'font.ttf'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'font.ttf') -Destination $fontPath -Force
$checksums = foreach ($asset in @($zipPath, $fontPath)) {
    $hash = (Get-FileHash -LiteralPath $asset -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $([IO.Path]::GetFileName($asset))"
}
[IO.File]::WriteAllText((Join-Path $destination 'SHA256SUMS.txt'), (($checksums -join "`n") + "`n"), [Text.UTF8Encoding]::new($false))
Write-Host "Release: $zipPath"
Write-Host ('Files: ' + ($files -join ', '))
