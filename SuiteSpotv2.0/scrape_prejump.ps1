# SuiteSpot Prejump Training Pack Scraper
# Scrapes all 2,301 training packs from prejump.com and exports as JSON
# 
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File scrape_prejump.ps1 -OutputPath "C:\path\to\output.json"
# Default output: %APPDATA%\bakkesmod\bakkesmod\data\SuiteTraining\prejump_packs.json
#
# Requirements:
#   - PowerShell 5.0+ (included with Windows 10+)
#   - Internet connection
#   - ~5MB disk space for output JSON
#
# Performance:
#   - Total time: ~2-3 minutes
#   - Pages: 231 (10 packs per page)
#   - Rate limit: 200ms between requests (polite scraping)

param(
    [string]$OutputPath = "$env:APPDATA\bakkesmod\bakkesmod\data\SuiteTraining\prejump_packs.json",
    [int]$TimeoutSec = 30,
    [switch]$QuietMode = $false
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# =====================================================================
# Helper Functions
# =====================================================================

function Write-Log {
    param(
        [string]$Message,
        [ValidateSet("Info", "Success", "Warning", "Error")][string]$Level = "Info",
        [switch]$NoNewline = $false
    )
    
    if ($QuietMode -and $Level -eq "Info") {
        return
    }
    
    $colors = @{
        "Info"    = "Cyan"
        "Success" = "Green"
        "Warning" = "Yellow"
        "Error"   = "Red"
    }
    
    Write-Host $Message -ForegroundColor $colors[$Level] -NoNewline:$NoNewline
}

function Invoke-PrejumpPageScrape {
    param(
        [int]$PageNumber,
        [int]$TotalPages
    )
    
    $url = "https://prejump.com/training-packs?page=$PageNumber"
    
    try {
        $response = Invoke-WebRequest -Uri $url `
            -UserAgent "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36" `
            -TimeoutSec $TimeoutSec `
            -ErrorAction Stop
        
        return $response.Content
    } catch {
        throw "Failed to fetch page $PageNumber : $($_.Exception.Message)"
    }
}

function Extract-PackDataFromHtml {
    param(
        [string]$HtmlContent
    )
    
    # Find the data-page attribute
    $pattern = 'data-page="([^"]*)"'
    $match = [regex]::Match($HtmlContent, $pattern)
    
    if (-not $match.Success) {
        throw "Could not find data-page attribute in HTML response"
    }
    
    $jsonString = $match.Groups[1].Value
    
    # Decode HTML entities
    $decoded = [System.Net.WebUtility]::HtmlDecode($jsonString)
    
    # Parse JSON
    $pageData = $decoded | ConvertFrom-Json
    
    if (-not $pageData.props.packs -or -not $pageData.props.packs.data) {
        throw "Could not find packs data in page JSON"
    }
    
    return @{
        Packs = $pageData.props.packs.data
        TotalCount = $pageData.props.packs.meta.total
        LastPage = $pageData.props.packs.meta.last_page
        CurrentPage = $pageData.props.packs.meta.current_page
    }
}

function Normalize-TrainingPack {
    param(
        [PSObject]$Pack
    )
    
    return @{
        name = $Pack.name
        code = $Pack.code
        creator = $Pack.creator
        creatorSlug = $Pack.creatorSlug
        difficulty = $Pack.difficulty
        shotCount = $Pack.shotCount
        tags = @($Pack.tags)  # Force array
        videoUrl = $Pack.videoUrl
        staffComments = $Pack.staffComments
        notes = $Pack.notes
        likes = $Pack.likes
        plays = $Pack.plays
        status = $Pack.status
    }
}

# =====================================================================
# Main Scraping Logic
# =====================================================================

Write-Log "╔════════════════════════════════════════════════════════╗" Info
Write-Log "`n║  SuiteSpot Prejump Training Pack Scraper              ║" Info
Write-Log "`n║  Scraping 2,301 packs from prejump.com...            ║" Info
Write-Log "`n╚════════════════════════════════════════════════════════╝`n" Info

Write-Log "Output: $OutputPath`n" Info

try {
    # ===== PHASE 1: Initial Fetch =====
    Write-Log "Phase 1: Fetching initial page..." Info
    $htmlContent = Invoke-PrejumpPageScrape -PageNumber 1 -TotalPages 1
    
    # Extract metadata and first page of packs
    $pageInfo = Extract-PackDataFromHtml -HtmlContent $htmlContent
    Write-Log "✓ Success" Success
    Write-Log " - Total packs available: $($pageInfo.TotalCount)" Info
    Write-Log " - Total pages to scrape: $($pageInfo.LastPage)" Info
    Write-Log " - First page loaded: $($pageInfo.Packs.Count) packs`n" Info
    
    # ===== PHASE 2: Multi-Page Scraping =====
    Write-Log "Phase 2: Scraping all pages..." Info
    Write-Log ""
    
    [System.Collections.Generic.List[PSObject]]$allPacks = @()
    $failedPages = @()
    
    for ($page = 1; $page -le $pageInfo.LastPage; $page++) {
        # Show progress
        $percent = [math]::Round(($page / $pageInfo.LastPage) * 100, 1)
        $barLength = 30
        $filledBars = [math]::Floor(($page / $pageInfo.LastPage) * $barLength)
        $progressBar = "█" * $filledBars + "░" * ($barLength - $filledBars)
        
        Write-Host -NoNewline "`r  [$progressBar] $percent% ($page/$($pageInfo.LastPage)) "
        
        try {
            if ($page -eq 1) {
                # Use already-fetched content from phase 1
                $packData = $pageInfo.Packs
            } else {
                # Fetch page
                $htmlContent = Invoke-PrejumpPageScrape -PageNumber $page -TotalPages $pageInfo.LastPage
                $pageData = Extract-PackDataFromHtml -HtmlContent $htmlContent
                $packData = $pageData.Packs
            }
            
            # Normalize and add packs
            foreach ($pack in $packData) {
                $normalized = Normalize-TrainingPack -Pack $pack
                $allPacks.Add($normalized) | Out-Null
            }
            
            # Polite rate limiting
            if ($page -lt $pageInfo.LastPage) {
                Start-Sleep -Milliseconds 200
            }
            
        } catch {
            Write-Host ""  # newline for error
            Write-Log "  ✗ Page $page failed: $($_.Exception.Message)" Warning
            $failedPages += $page
            Start-Sleep -Milliseconds 500  # Extra delay on error
        }
    }
    
    Write-Host ""  # Final newline
    Write-Log "✓ Scraping complete`n" Success
    
    # ===== PHASE 3: Validation & Reporting =====
    Write-Log "Phase 3: Validating and preparing output..." Info
    
    $uniquePacks = $allPacks | Group-Object -Property code | Where-Object { $_.Count -eq 1 } | Select-Object -ExpandProperty Group
    
    Write-Log "  Total packs scraped: $($allPacks.Count)" Info
    Write-Log "  Unique packs: $($uniquePacks.Count)" Info
    
    if ($failedPages.Count -gt 0) {
        Write-Log "  Failed pages: $($failedPages.Count)" Warning
        Write-Log "    Pages: $($failedPages -join ', ')" Warning
    }
    
    # Validate minimum success
    if ($allPacks.Count -lt 2000) {
        Write-Log "  ⚠ WARNING: Expected ~2,301 packs but only got $($allPacks.Count)" Warning
        Write-Log "  Continuing with available data..." Warning
    }
    
    Write-Log "`n"
    
    # ===== PHASE 4: Output Generation =====
    Write-Log "Phase 4: Generating output JSON..." Info
    
    # Create output structure
    $output = @{
        version = "1.0.0"
        lastUpdated = (Get-Date -Format "o")
        source = "https://prejump.com/training-packs"
        totalPacks = $uniquePacks.Count
        packs = $uniquePacks
    }
    
    # Convert to JSON with proper depth
    $json = $output | ConvertTo-Json -Depth 10 -Compress:$false
    
    # Ensure output directory exists
    $outputDir = Split-Path -Parent $OutputPath
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Path $outputDir -Force -ErrorAction Stop | Out-Null
        Write-Log "  Created output directory: $outputDir" Info
    }
    
    # Write to file
    $json | Out-File -FilePath $OutputPath -Encoding UTF8 -Force -ErrorAction Stop
    
    Write-Log "✓ JSON generated and saved" Success
    
    $fileSize = (Get-Item $OutputPath).Length / 1MB
    Write-Log "  File size: $([math]::Round($fileSize, 2)) MB" Info
    Write-Log "  Location: $OutputPath`n" Info
    
    # ===== PHASE 5: Statistics =====
    Write-Log "Phase 5: Generating statistics..." Info
    
    # Difficulty distribution
    Write-Log "`n  Packs by Difficulty:" Info
    $difficulties = $uniquePacks.difficulty | Group-Object | Sort-Object Count -Descending
    foreach ($diff in $difficulties) {
        $pct = [math]::Round(($diff.Count / $uniquePacks.Count) * 100, 1)
        Write-Log "    $($diff.Name -PadRight 20) : $($diff.Count -ToString "0000") ($pct%)" Info
    }
    
    # Tags distribution
    Write-Log "`n  Tags by Category:" Info
    $allTags = @()
    foreach ($pack in $uniquePacks) {
        $allTags += $pack.tags
    }
    
    # Note: We can't extract category from this data, so we'll just count unique tags
    $uniqueTags = $allTags | Select-Object -Unique | Measure-Object | Select-Object -ExpandProperty Count
    Write-Log "    Total unique tags: $uniqueTags" Info
    
    # Creator distribution (top 10)
    Write-Log "`n  Top 10 Contributors:" Info
    $topCreators = $uniquePacks.creator | Group-Object | Sort-Object Count -Descending | Select-Object -First 10
    foreach ($creator in $topCreators) {
        Write-Log "    $($creator.Name -PadRight 20) : $($creator.Count) packs" Info
    }
    
    Write-Log "`n"
    Write-Log "╔════════════════════════════════════════════════════════╗" Info
    Write-Log "`n║  ✓ SUCCESS - Scraping complete!                       ║" Info
    Write-Log "`n║  Scraped: $($uniquePacks.Count) training packs from Prejump    ║" Info
    Write-Log "`n║  Output: $(Split-Path -Leaf $OutputPath)" Info
    Write-Log "`n╚════════════════════════════════════════════════════════╝`n" Success
    
    exit 0

} catch {
    Write-Log ""
    Write-Log "╔════════════════════════════════════════════════════════╗" Error
    Write-Log "`n║  ✗ ERROR - Scraping failed!                           ║" Error
    Write-Log "`n║  $($_.Exception.Message -PadRight 50) ║" Error
    Write-Log "`n╚════════════════════════════════════════════════════════╝`n" Error
    
    Write-Log "Stack trace:`n$($_.ScriptStackTrace)`n" Error
    
    exit 1
}
