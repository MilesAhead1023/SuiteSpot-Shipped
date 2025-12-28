# Deploy scraper and data to BakkesMod data folder
# Structure: data\SuiteSpot\scrape_prejump.ps1
#            data\SuiteSpot\SuiteTraining\prejump_packs.json

$suiteSpotDir = Join-Path $env:APPDATA "bakkesmod\bakkesmod\data\SuiteSpot"
$trainingDir = Join-Path $suiteSpotDir "SuiteTraining"

# Create directories
if (-not (Test-Path $suiteSpotDir)) {
    New-Item -ItemType Directory -Path $suiteSpotDir -Force | Out-Null
    Write-Host "Created: $suiteSpotDir"
}
if (-not (Test-Path $trainingDir)) {
    New-Item -ItemType Directory -Path $trainingDir -Force | Out-Null
    Write-Host "Created: $trainingDir"
}

# Deploy files
$scriptPath = Join-Path $PSScriptRoot "scrape_prejump.ps1"
$jsonPath = Join-Path $PSScriptRoot "data\prejump_packs.json"

Copy-Item $scriptPath $suiteSpotDir -Force
Write-Host "Deployed: scrape_prejump.ps1 -> $suiteSpotDir"

Copy-Item $jsonPath $trainingDir -Force
Write-Host "Deployed: prejump_packs.json -> $trainingDir"

Write-Host ""
Write-Host "Deployment complete:"
Get-ChildItem $suiteSpotDir -Recurse -File | Select-Object FullName, Length | Format-Table -AutoSize
