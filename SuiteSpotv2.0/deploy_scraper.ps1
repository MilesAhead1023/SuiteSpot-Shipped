# Deploy scraper to BakkesMod data folder
$dest = Join-Path $env:APPDATA "bakkesmod\bakkesmod\data\SuiteSpot"

if (-not (Test-Path $dest)) {
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
    Write-Host "Created: $dest"
}

$scriptPath = Join-Path $PSScriptRoot "scrape_prejump.ps1"
$jsonPath = Join-Path $PSScriptRoot "data\prejump_packs.json"

Copy-Item $scriptPath $dest -Force
Write-Host "Deployed: scrape_prejump.ps1"

Copy-Item $jsonPath $dest -Force
Write-Host "Deployed: prejump_packs.json"

Write-Host ""
Write-Host "Files in $dest :"
Get-ChildItem $dest -File | Select-Object Name, Length | Format-Table -AutoSize
