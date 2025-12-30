param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

# Relative path to IMGUI from the git root (assuming script runs from tools/guardrails)
# $RootPath resolves to SuiteSpotv2.0. 
# We need to check relative to where git runs, or just check the specific folder.

$imguiPath = Join-Path $RootPath "IMGUI"

# Check if git detects changes in this specific path
# We use --porcelain to get machine-readable output. 
# If output is empty, no changes.

$gitStatus = git status --porcelain "$imguiPath" 2>&1

if ($gitStatus) {
    Write-Host "IMGUI INTEGRITY VIOLATION: Modifications detected in '$imguiPath'." -ForegroundColor Red
    Write-Host "The IMGUI folder is read-only. Do not modify library files." -ForegroundColor Yellow
    Write-Host $gitStatus
    exit 1
} else {
    Write-Host "IMGUI Integrity Check Passed." -ForegroundColor Green
    exit 0
}
