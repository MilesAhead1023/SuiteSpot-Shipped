param(
    [string]$RootPath = "$PSScriptRoot\..\.."
)

$files = Get-ChildItem -Path $RootPath -Include "*.cpp","*.h" -Recurse -ErrorAction SilentlyContinue | 
    Where-Object { $_.FullName -notmatch "IMGUI" -and $_.FullName -notmatch "BakkesModWiki" -and $_.FullName -notmatch "tools" }

$wrappers = New-Object System.Collections.Generic.HashSet[string]
$methods = New-Object System.Collections.Generic.HashSet[string]

# Regex to find Wrapper types (e.g., CarWrapper, ServerWrapper)
$wrapperRegex = [regex]"\b([a-zA-Z0-9]+Wrapper)\b"

# Regex to find method calls on likely wrappers (e.g., wrapper->GetBall, wrapper.GetData)
# This is a heuristic: it looks for -> or . followed by a CapitalizedMethodName
$methodRegex = [regex]"(?:->|\.)([A-Z][a-zA-Z0-9]+)\("

foreach ($file in $files) {
    $content = Get-Content -Path $file.FullName
    foreach ($line in $content) {
        # Find Wrappers
        $wMatches = $wrapperRegex.Matches($line)
        foreach ($match in $wMatches) {
            [void]$wrappers.Add($match.Groups[1].Value)
        }

        # Find Methods (heuristic)
        $mMatches = $methodRegex.Matches($line)
        foreach ($match in $mMatches) {
            # Filter out common C++ standard lib or local methods if needed, 
            # but for now we capture all capitalized method calls as candidates.
            [void]$methods.Add($match.Groups[1].Value)
        }
    }
}

Write-Host "=== Detected Wrapper Types ==="
$sortedWrappers = $wrappers | Sort-Object
foreach ($w in $sortedWrappers) { Write-Host $w }

Write-Host "`n=== Detected Potential API Methods ==="
$sortedMethods = $methods | Sort-Object
foreach ($m in $sortedMethods) { Write-Host $m }
