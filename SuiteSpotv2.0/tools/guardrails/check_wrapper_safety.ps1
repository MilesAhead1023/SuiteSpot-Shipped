param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

$headerFiles = Get-ChildItem -Path $RootPath -Filter "*.h" -Recurse -ErrorAction SilentlyContinue | Where-Object { $_.FullName -notmatch "IMGUI" -and $_.Name -ne "SuiteSpot.h" }

$warnings = 0

foreach ($file in $headerFiles) {
    $content = Get-Content -Path $file.FullName
    $lineNum = 0
    foreach ($line in $content) {
        $lineNum++
        # Look for member declaration: TypeWrapper name; or TypeWrapper* name;
        # Exclude function arguments (parentheses)
        if ($line -match '\b(\w+Wrapper)\s+(\*?)\s*\w+\s*;' -and $line -notmatch '\(') {
            Write-Host "WRAPPER SAFETY WARNING: $($file.Name):$lineNum stores '$($matches[1])'. Wrappers should usually be transient." -ForegroundColor Yellow
            $warnings++
        }
    }
}

if ($warnings -eq 0) {
    Write-Host "Wrapper Safety Check Passed." -ForegroundColor Green
    exit 0
} else {
    Write-Host "Wrapper Safety: Found $warnings warnings." -ForegroundColor Yellow
    # Don't fail the build, just warn
    exit 0
}
