# Generate suitespot.set file for BakkesMod settings UI
# Format: type|label|cvar_name|min|max|description
# Types: 1=Checkbox, 3=Number slider, 8=Separator, 9=Comment

$settingsDir = "$env:APPDATA\bakkesmod\bakkesmod\plugins\settings"
$setFile = "$settingsDir\suitespot.set"

# Create directory if it doesn't exist
if (-not (Test-Path $settingsDir)) {
    New-Item -ItemType Directory -Path $settingsDir -Force | Out-Null
    Write-Host "[PostBuild] Created settings directory: $settingsDir"
}

# Generate the .set file content
$content = @"
SuiteSpot
1|Enable SuiteSpot|suitespot_enabled
8|
3|Map Type (0=Freeplay, 1=Training, 2=Workshop)|suitespot_map_type|0|2
1|Auto-Queue After Map Load|suitespot_auto_queue
1|Shuffle Training Packs|suitespot_training_shuffle
8|
9|Delays (seconds)
3|Queue Delay|suitespot_delay_queue_sec|0|300
3|Freeplay Delay|suitespot_delay_freeplay_sec|0|300
3|Training Delay|suitespot_delay_training_sec|0|300
3|Workshop Delay|suitespot_delay_workshop_sec|0|300
8|
9|Overlay Settings
1|Show Test Overlay|ss_testoverlay
3|Overlay Width|overlay_width|400|1600
3|Overlay Height|overlay_height|200|800
3|Overlay Transparency|overlay_alpha|0|1
3|Display Duration|overlay_duration|5|60
8|
9|Team Colors (Hue)
3|Blue Team Hue|blue_team_hue|0|360
3|Orange Team Hue|orange_team_hue|0|360
8|
9|Use 'togglemenu SuiteSpot' to open the control window
9|Author: Flicks Creations
"@

# Write the file
try {
    $content | Out-File -FilePath $setFile -Encoding ASCII -Force
    Write-Host "[PostBuild] Generated settings file: $setFile"
    exit 0
} catch {
    Write-Host "[PostBuild] ERROR: Failed to generate settings file: $_"
    exit 1
}