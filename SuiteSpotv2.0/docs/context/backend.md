# Backend Subsystem Context

## Purpose
Manages map discovery, training pack scraping, persistence, and selection logic.

## Key Components
- **MapManager**: Handles Workshop map discovery and path resolution.
- **TrainingPackManager**: Manages the JSON cache of training packs, scraping via PowerShell, and shuffle bag logic.

## Data Flow
1. **Scrape**: `TrainingPackManager` calls `scrape_prejump.ps1`.
2. **Load**: `onLoad` triggers `TrainingPackManager::LoadPacksFromFile`.
3. **Discover**: `MapManager` scans local mods folders for `.upk` files.

## Files
- `MapManager.h/cpp`
- `TrainingPackManager.h/cpp`
- `MapList.h/cpp`

## Triggers
Keywords: Workshop, Training Pack, Scrape, Shuffle, UPK.
Files: `MapManager.*`, `TrainingPackManager.*`, `MapList.*`.
