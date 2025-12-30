param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

$cppFiles = Get-ChildItem -Path $RootPath -Filter "*.cpp" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch "IMGUI" }

$failed = $false

foreach ($file in $cppFiles) {
    $content = Get-Content -Path $file.FullName
    $firstInclude = $null
    
    foreach ($line in $content) {
        if ($line -match '^\s*#include\s*["<](.*)[">]') {
            $firstInclude = $matches[1]
            break
        }
    }

    if ($firstInclude -ne "pch.h") {
        Write-Host "PCH VIOLATION: $($file.Name) does not include 'pch.h' first. Found: '$firstInclude'" -ForegroundColor Red
        $failed = $true
    }
}

if (-not $failed) {
    Write-Host "PCH Check Passed." -ForegroundColor Green
    exit 0
} else {
    exit 1
}
