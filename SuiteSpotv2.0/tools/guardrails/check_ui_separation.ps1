param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

# Files allowed to include ImGui
$allowedPattern = ".*UI\.(cpp|h)|Source\.cpp|pch\.(h|cpp)"

$files = Get-ChildItem -Path $RootPath -Include "*.cpp","*.h" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch "IMGUI" }

$failed = $false

foreach ($file in $files) {
    if ($file.Name -match $allowedPattern) {
        continue
    }

    $content = Get-Content -Path $file.FullName
    $lineNum = 0
    foreach ($line in $content) {
        $lineNum++
        if ($line -match '#include\s*["<]imgui.*[">]') {
            Write-Host "UI SEPARATION VIOLATION: $($file.Name):$lineNum includes ImGui. Logic should be separated from UI." -ForegroundColor Red
            $failed = $true
        }
    }
}

if (-not $failed) {
    Write-Host "UI Separation Check Passed." -ForegroundColor Green
} else {
    exit 1
}
