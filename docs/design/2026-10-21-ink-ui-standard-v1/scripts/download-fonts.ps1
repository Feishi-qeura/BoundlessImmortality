$ErrorActionPreference = 'Stop'
$uiPackRoot = Split-Path -Parent $PSScriptRoot
$uiManifestPath = Join-Path $uiPackRoot 'fonts/font-manifest.json'
$uiManifest = Get-Content -LiteralPath $uiManifestPath -Raw | ConvertFrom-Json
# Restore exact delivered versions; do not query moving main/latest refs.
foreach ($uiEntry in $uiManifest) {
    $uiTarget = [IO.Path]::GetFullPath((Join-Path $uiPackRoot $uiEntry.path))
    $uiAllowed = [IO.Path]::GetFullPath((Join-Path $uiPackRoot 'fonts')) + [IO.Path]::DirectorySeparatorChar
    if (-not $uiTarget.StartsWith($uiAllowed,[StringComparison]::OrdinalIgnoreCase)) { throw 'Font path escapes package' }
    if ($uiEntry.commit -notmatch '^[a-f0-9]{40}$' -or $uiEntry.sha256 -notmatch '^[a-f0-9]{64}$') { throw 'Invalid pinned manifest' }
    if (Test-Path -LiteralPath $uiTarget) {
        if ((Get-FileHash -LiteralPath $uiTarget -Algorithm SHA256).Hash.ToLowerInvariant() -eq $uiEntry.sha256) {
            Write-Output ('Verified existing: ' + $uiEntry.path)
            continue
        }
    }
    $uiUri = [uri]$uiEntry.transfer_url
    if ($uiUri.Scheme -ne 'https' -or $uiUri.Host -ne 'api.github.com') { throw 'Unexpected font source' }
    New-Item -ItemType Directory -Path (Split-Path -Parent $uiTarget) -Force | Out-Null
    $uiPart = $uiTarget + '.download'
    try {
        Invoke-WebRequest -Uri $uiUri -Headers @{'User-Agent'='BoundlessImmortality-UI-Design';'Accept'='application/vnd.github.raw+json'} -OutFile $uiPart
        if ((Get-Item -LiteralPath $uiPart).Length -ne $uiEntry.bytes) { throw ('Wrong byte count: ' + $uiEntry.path) }
        if ((Get-FileHash -LiteralPath $uiPart -Algorithm SHA256).Hash.ToLowerInvariant() -ne $uiEntry.sha256) { throw ('Hash mismatch: ' + $uiEntry.path) }
        Move-Item -LiteralPath $uiPart -Destination $uiTarget -Force
        Write-Output ('Restored verified: ' + $uiEntry.path)
    } finally {
        if (Test-Path -LiteralPath $uiPart) { Remove-Item -LiteralPath $uiPart }
    }
}
