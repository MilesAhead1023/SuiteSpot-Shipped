param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

$cppFiles = Get-ChildItem -Path $RootPath -Filter "*.cpp" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch "IMGUI" -and $_.Name -ne "SettingsSync.cpp" }

$failed = $false

foreach ($file in $cppFiles) {
    $content = Get-Content -Path $file.FullName
    $lineNum = 0
    foreach ($line in $content) {
        $lineNum++
        if ($line -match 'registerCvar') {
            Write-Host "CVAR VIOLATION: $($file.Name):$lineNum calls 'registerCvar'. Move to SettingsSync::RegisterAllCVars." -ForegroundColor Red
            $failed = $true
        }
    }
}

if (-not $failed) {
    Write-Host "CVar Check Passed." -ForegroundColor Green
    exit 0
} else {
    exit 1
}
