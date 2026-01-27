#include "pch.h"
#include "WorkshopDownloader.h"
#include <fstream>
#include <sstream>

// TODO: Add cpr/curl includes once dependencies are installed
// #include <cpr/cpr.h>

void WorkshopDownloader::SearchMaps(const std::string& keywords, int page)
{
    if (isSearching) {
        LOG("WorkshopDownloader: Search already in progress");
        return;
    }
    
    isSearching = true;
    currentPage = page;
    searchResults.clear();
    
    // TODO: Implement API call to rocketleaguemaps.us
    // For now, just a placeholder
    LOG("WorkshopDownloader: Searching for '{}' (page {})", keywords, page);
    
    // Placeholder - will implement with HTTP library
    isSearching = false;
}

void WorkshopDownloader::DownloadMap(const WorkshopSearchResult& result,
                                      const std::filesystem::path& destinationFolder,
                                      std::function<void(const DownloadProgress&)> progressCallback)
{
    if (isDownloading) {
        LOG("WorkshopDownloader: Download already in progress");
        return;
    }
    
    isDownloading = true;
    downloadProgress = DownloadProgress();
    downloadProgress.mapName = result.name;
    
    LOG("WorkshopDownloader: Starting download of '{}'", result.name);
    
    try {
        // Create map folder
        auto mapFolder = destinationFolder / result.name;
        if (!std::filesystem::exists(mapFolder)) {
            std::filesystem::create_directories(mapFolder);
        }
        
        // Download zip file
        auto zipPath = mapFolder / (result.name + ".zip");
        
        auto progressUpdate = [this, progressCallback](size_t downloaded, size_t total) {
            downloadProgress.bytesDownloaded = downloaded;
            downloadProgress.totalBytes = total;
            downloadProgress.percentComplete = total > 0 ? (int)((downloaded * 100) / total) : 0;
            
            if (progressCallback) {
                progressCallback(downloadProgress);
            }
        };
        
        if (!DownloadFile(result.downloadUrl, zipPath, progressUpdate)) {
            downloadProgress.hasFailed = true;
            downloadProgress.errorMessage = "Download failed";
            isDownloading = false;
            return;
        }
        
        // Extract zip
        if (!ExtractZipFile(zipPath, mapFolder)) {
            downloadProgress.hasFailed = true;
            downloadProgress.errorMessage = "Extraction failed";
            isDownloading = false;
            return;
        }
        
        // Create metadata JSON
        CreateMapMetadata(mapFolder, result);
        
        // Clean up zip file
        std::filesystem::remove(zipPath);
        
        downloadProgress.isComplete = true;
        LOG("WorkshopDownloader: Successfully downloaded '{}'", result.name);
        
    } catch (const std::exception& e) {
        downloadProgress.hasFailed = true;
        downloadProgress.errorMessage = e.what();
        LOG("WorkshopDownloader: Download error: {}", e.what());
    }
    
    isDownloading = false;
}

void WorkshopDownloader::DownloadPreviewImage(WorkshopSearchResult& result)
{
    if (result.previewUrl.empty()) {
        return;
    }
    
    // Create preview cache folder
    auto previewFolder = workshopFolder / "previews";
    if (!std::filesystem::exists(previewFolder)) {
        std::filesystem::create_directories(previewFolder);
    }
    
    // Download image
    auto imagePath = previewFolder / (result.id + ".jpg");
    if (std::filesystem::exists(imagePath)) {
        // Already cached
        result.previewImagePath = imagePath;
        result.previewImage = std::make_shared<ImageWrapper>(imagePath, false, true);
        result.isPreviewLoaded = true;
        return;
    }
    
    // TODO: Download image using HTTP library
    LOG("WorkshopDownloader: Downloading preview for '{}'", result.name);
}

bool WorkshopDownloader::DownloadFile(const std::string& url, 
                                       const std::filesystem::path& outputPath,
                                       std::function<void(size_t, size_t)> progressCallback)
{
    // TODO: Implement using cpr/curl once dependencies are added
    LOG("WorkshopDownloader: DownloadFile placeholder - {}", url);
    return false;
}

bool WorkshopDownloader::ExtractZipFile(const std::filesystem::path& zipPath, 
                                         const std::filesystem::path& extractTo)
{
    // TODO: Implement using zlib once dependency is added
    LOG("WorkshopDownloader: ExtractZipFile placeholder - {}", zipPath.string());
    return false;
}

void WorkshopDownloader::CreateMapMetadata(const std::filesystem::path& mapFolder, 
                                            const WorkshopSearchResult& result)
{
    nlohmann::json metadata;
    metadata["name"] = result.name;
    metadata["author"] = result.author;
    metadata["description"] = result.description;
    metadata["previewUrl"] = result.previewUrl;
    metadata["downloadUrl"] = result.downloadUrl;
    metadata["id"] = result.id;
    
    auto jsonPath = mapFolder / "workshop_info.json";
    std::ofstream file(jsonPath);
    if (file.is_open()) {
        file << metadata.dump(4);
        file.close();
        LOG("WorkshopDownloader: Created metadata at {}", jsonPath.string());
    } else {
        LOG("WorkshopDownloader: Failed to create metadata file");
    }
}

std::string WorkshopDownloader::CleanHTML(const std::string& html)
{
    // Remove HTML tags
    std::string cleaned = html;
    size_t pos = 0;
    while ((pos = cleaned.find('<')) != std::string::npos) {
        size_t endPos = cleaned.find('>', pos);
        if (endPos != std::string::npos) {
            cleaned.erase(pos, endPos - pos + 1);
        } else {
            break;
        }
    }
    return cleaned;
}
