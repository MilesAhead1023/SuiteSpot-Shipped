$ErrorActionPreference = "Stop"

Write-Host "Running SuiteSpot Guardrails..." -ForegroundColor Cyan

$scripts = @(
    "check_pch.ps1",
    "check_wrapper_safety.ps1",
    "check_cvars.ps1",
    "check_ui_separation.ps1",
    "check_imgui_integrity.ps1"
)

$failed = $false

foreach ($script in $scripts) {
    Write-Host "Execting $script..." -ForegroundColor Gray
    try {
        & "$PSScriptRoot\$script"
        if ($LASTEXITCODE -ne 0) {
            $failed = $true
        }
    } catch {
        Write-Host "Error running $script : $_" -ForegroundColor Red
        $failed = $true
    }
}

Write-Host "Executing Build Integrity Check (verify.bat)..." -ForegroundColor Gray
cmd.exe /c "cd /d $PSScriptRoot\..\.. && verify.bat"
if ($LASTEXITCODE -ne 0) {
    $failed = $true
}

if ($failed) {
    Write-Host "`nGUARDRAILS FAILED. Please fix violations before committing." -ForegroundColor Red
    exit 1
} else {
    Write-Host "`nAll Guardrails Passed." -ForegroundColor Green
    exit 0
}
