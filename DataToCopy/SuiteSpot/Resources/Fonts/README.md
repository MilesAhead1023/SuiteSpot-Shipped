# SuiteSpot Font Directory

## Required Font

**Place `Roboto-Regular.ttf` in this directory** for the modern UI theme to use a professional font.

### Where to get Roboto-Regular.ttf

1. **Google Fonts (Recommended)**:
   - Visit: https://fonts.google.com/specimen/Roboto
   - Click "Download family"
   - Extract the ZIP
   - Copy `Roboto-Regular.ttf` to this directory

2. **Alternative**: If you already have the font file, simply copy it here.

### Fallback Behavior

If `Roboto-Regular.ttf` is not found in this directory, SuiteSpot will automatically fall back to ImGui's default built-in font (ProggyClean). The plugin will still work perfectly, just with a different font.

### Installation Path

After building, this directory will be copied to:
```
%APPDATA%\bakkesmod\bakkesmod\data\SuiteSpot\Resources\Fonts\
```

The plugin will look for the font there at runtime.
