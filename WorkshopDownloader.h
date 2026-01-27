#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "MapList.h"
#include "logging.h"
#include "IMGUI/json.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <functional>

/*
 * ======================================================================================
 * WORKSHOP DOWNLOADER: SEARCH, DOWNLOAD, AND MANAGE WORKSHOP MAPS
 * ======================================================================================
 * 
 * WHAT IS THIS?
 * This class handles searching for, downloading, and managing workshop maps from
 * online sources like rocketleaguemaps.us. It provides integration with the Steam
 * Workshop ecosystem without requiring Steam.
 *
 * WHY IS IT HERE?
 * Users need an easy way to discover and download workshop maps directly from within
 * the plugin, rather than manually browsing websites and extracting files.
 *
 * HOW DOES IT WORK?
 * 1. Search: Query rocketleaguemaps.us API for maps matching keywords
 * 2. Download: Fetch map files (zip archives) from URLs
 * 3. Extract: Unzip downloaded maps into the workshop folder
 * 4. Metadata: Create JSON files with map info (name, author, description, preview)
 * 5. Integration: Discovered maps appear in the workshop dropdown automatically
 */

// Workshop map search result from rocketleaguemaps.us
struct WorkshopSearchResult
{
    std::string id;              // Map ID from API
    std::string name;            // Map display name
    std::string author;          // Creator name
    std::string description;     // Map description (HTML cleaned)
    std::string downloadUrl;     // Direct download link
    std::string previewUrl;      // Preview image URL
    std::string sizeBytes;       // File size in bytes
    std::filesystem::path previewImagePath;  // Local cached preview image
    std::shared_ptr<ImageWrapper> previewImage;  // Loaded ImGui texture
    bool isPreviewLoaded = false;
};

// Download progress information for UI updates
struct DownloadProgress
{
    std::string mapName;
    size_t bytesDownloaded = 0;
    size_t totalBytes = 0;
    int percentComplete = 0;
    bool isComplete = false;
    bool hasFailed = false;
    std::string errorMessage;
};

class WorkshopDownloader
{
public:
    WorkshopDownloader() = default;
    
    // Search workshop maps
    void SearchMaps(const std::string& keywords, int page = 0);
    bool IsSearching() const { return isSearching; }
    const std::vector<WorkshopSearchResult>& GetSearchResults() const { return searchResults; }
    int GetTotalPages() const { return totalPages; }
    
    // Download workshop map
    void DownloadMap(const WorkshopSearchResult& result, 
                     const std::filesystem::path& destinationFolder,
                     std::function<void(const DownloadProgress&)> progressCallback = nullptr);
    bool IsDownloading() const { return isDownloading; }
    const DownloadProgress& GetDownloadProgress() const { return downloadProgress; }
    
    // Preview image management
    void DownloadPreviewImage(WorkshopSearchResult& result);
    
    // Configuration
    void SetWorkshopFolder(const std::filesystem::path& folder) { workshopFolder = folder; }
    std::filesystem::path GetWorkshopFolder() const { return workshopFolder; }
    
private:
    // Internal download helpers
    bool DownloadFile(const std::string& url, const std::filesystem::path& outputPath,
                      std::function<void(size_t, size_t)> progressCallback = nullptr);
    bool ExtractZipFile(const std::filesystem::path& zipPath, const std::filesystem::path& extractTo);
    void CreateMapMetadata(const std::filesystem::path& mapFolder, const WorkshopSearchResult& result);
    
    // API helpers
    std::string CleanHTML(const std::string& html);
    
    // State
    std::filesystem::path workshopFolder;
    std::vector<WorkshopSearchResult> searchResults;
    bool isSearching = false;
    bool isDownloading = false;
    int totalPages = 0;
    int currentPage = 0;
    DownloadProgress downloadProgress;
    
    // API endpoints
    const std::string RLMAPS_SEARCH_URL = "https://celab.jetfox.ovh/api/v4/projects/?search=";
};
