#include "pch.h"
#include "WorkshopDownloader.h"
#include <fstream>
#include <sstream>

void WorkshopDownloader::SearchMaps(const std::string& keywords, int page)
{
    if (isSearching) {
        LOG("WorkshopDownloader: Search already in progress");
        return;
    }
    
    isSearching = true;
    currentPage = page;
    searchResults.clear();
    
    LOG("WorkshopDownloader: Searching for '{}' (page {})", keywords, page);
    
    // Search in a separate thread to avoid blocking UI
    std::thread searchThread([this, keywords, page]() {
        try {
            std::string searchUrl = RLMAPS_SEARCH_URL + keywords + "&page=" + std::to_string(page);
            
            CurlRequest req;
            req.url = searchUrl;
            
            HttpWrapper::SendCurlJsonRequest(req, [this](int code, nlohmann::json response) {
                if (code == 200 && response.is_array()) {
                    LOG("WorkshopDownloader: Found {} results", response.size());
                    
                    for (const auto& item : response) {
                        WorkshopSearchResult result;
                        result.id = item.value("id", "");
                        result.name = item.value("name", "Unknown");
                        result.author = item.value("creator", "Unknown");
                        result.description = CleanHTML(item.value("description", ""));
                        
                        // Extract download info from releases
                        if (item.contains("releases") && item["releases"].is_array() && !item["releases"].empty()) {
                            auto& release = item["releases"][0];
                            result.downloadUrl = release.value("url", "");
                            result.zipFileName = release.value("name", "map.zip");
                        }
                        
                        result.previewUrl = item.value("image_url", "");
                        result.sizeBytes = item.value("file_size", "0");
                        
                        searchResults.push_back(result);
                    }
                } else {
                    LOG("WorkshopDownloader: Search failed with code {}", code);
                }
                
                isSearching = false;
            });
            
        } catch (const std::exception& e) {
            LOG("WorkshopDownloader: Search error: {}", e.what());
            isSearching = false;
        }
    });
    
    searchThread.detach();
}

void WorkshopDownloader::DownloadMap(const WorkshopSearchResult& result,
                                      const std::filesystem::path& destinationFolder,
                                      std::function<void(const DownloadProgress&)> progressCallback)
{
    if (isDownloading) {
        LOG("WorkshopDownloader: Download already in progress");
        return;
    }
    
    if (result.downloadUrl.empty()) {
        LOG("WorkshopDownloader: No download URL for '{}'", result.name);
        return;
    }
    
    isDownloading = true;
    downloadProgress = DownloadProgress();
    downloadProgress.mapName = result.name;
    
    LOG("WorkshopDownloader: Starting download of '{}'", result.name);
    
    // Download in a separate thread
    std::thread downloadThread([this, result, destinationFolder, progressCallback]() {
        try {
            // Sanitize map name for filesystem
            std::string safeName = SanitizeMapName(result.name);
            
            // Create map folder
            auto mapFolder = destinationFolder / safeName;
            if (!std::filesystem::exists(mapFolder)) {
                std::filesystem::create_directories(mapFolder);
            }
            
            // Create metadata JSON first
            CreateMapMetadata(mapFolder, result);
            
            // Download zip file
            auto zipPath = mapFolder / result.zipFileName;
            
            CurlRequest req;
            req.url = result.downloadUrl;
            req.progress_function = [this, progressCallback](double fileSize, double downloaded, ...) {
                downloadProgress.bytesDownloaded = downloaded;
                downloadProgress.totalBytes = fileSize;
                downloadProgress.percentComplete = fileSize > 0 ? (int)((downloaded * 100.0) / fileSize) : 0;
                
                if (progressCallback) {
                    progressCallback(downloadProgress);
                }
            };
            
            HttpWrapper::SendCurlRequest(req, [this, zipPath, mapFolder, result](int code, char* data, size_t size) {
                if (code == 200) {
                    // Write zip file
                    std::ofstream outFile(zipPath, std::ios::binary);
                    if (outFile) {
                        outFile.write(data, size);
                        outFile.close();
                        
                        LOG("WorkshopDownloader: Downloaded {} bytes to {}", size, zipPath.string());
                        
                        // Extract zip
                        ExtractZipFile(zipPath, mapFolder);
                        
                        // Rename .udk to .upk if needed
                        RenameUdkToUpk(mapFolder);
                        
                        // Clean up zip file
                        std::filesystem::remove(zipPath);
                        
                        downloadProgress.isComplete = true;
                        LOG("WorkshopDownloader: Successfully downloaded and extracted '{}'", result.name);
                    } else {
                        downloadProgress.hasFailed = true;
                        downloadProgress.errorMessage = "Failed to write zip file";
                        LOG("WorkshopDownloader: Failed to write zip file");
                    }
                } else {
                    downloadProgress.hasFailed = true;
                    downloadProgress.errorMessage = "HTTP error " + std::to_string(code);
                    LOG("WorkshopDownloader: Download failed with code {}", code);
                }
                
                isDownloading = false;
            });
            
        } catch (const std::exception& e) {
            downloadProgress.hasFailed = true;
            downloadProgress.errorMessage = e.what();
            LOG("WorkshopDownloader: Download error: {}", e.what());
            isDownloading = false;
        }
    });
    
    downloadThread.detach();
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
    
    // Check if already cached
    auto imagePath = previewFolder / (result.id + ".jpg");
    if (std::filesystem::exists(imagePath)) {
        result.previewImagePath = imagePath;
        result.previewImage = std::make_shared<ImageWrapper>(imagePath, false, true);
        result.isPreviewLoaded = true;
        return;
    }
    
    // Download image
    LOG("WorkshopDownloader: Downloading preview for '{}'", result.name);
    
    CurlRequest req;
    req.url = result.previewUrl;
    
    HttpWrapper::SendCurlRequest(req, [this, imagePath, &result](int code, char* data, size_t size) {
        if (code == 200) {
            std::ofstream outFile(imagePath, std::ios::binary);
            if (outFile) {
                outFile.write(data, size);
                outFile.close();
                
                result.previewImagePath = imagePath;
                result.previewImage = std::make_shared<ImageWrapper>(imagePath, false, true);
                result.isPreviewLoaded = true;
                
                LOG("WorkshopDownloader: Preview downloaded to {}", imagePath.string());
            }
        }
    });
}

void WorkshopDownloader::ExtractZipFile(const std::filesystem::path& zipPath, 
                                         const std::filesystem::path& extractTo)
{
    LOG("WorkshopDownloader: Extracting {} to {}", zipPath.string(), extractTo.string());
    
    // Use PowerShell Expand-Archive cmdlet
    std::string command = "powershell.exe -Command \"Expand-Archive -LiteralPath '" + 
                          zipPath.string() + "' -DestinationPath '" + 
                          extractTo.string() + "' -Force\"";
    
    int result = system(command.c_str());
    
    if (result == 0) {
        LOG("WorkshopDownloader: Extraction successful");
    } else {
        LOG("WorkshopDownloader: Extraction failed with code {}", result);
    }
}

void WorkshopDownloader::CreateMapMetadata(const std::filesystem::path& mapFolder, 
                                            const WorkshopSearchResult& result)
{
    nlohmann::json metadata;
    metadata["Title"] = result.name;
    metadata["Author"] = result.author;
    metadata["Description"] = result.description;
    metadata["PreviewUrl"] = result.previewUrl;
    metadata["DownloadUrl"] = result.downloadUrl;
    metadata["ID"] = result.id;
    
    std::string safeName = SanitizeMapName(result.name);
    auto jsonPath = mapFolder / (safeName + ".json");
    
    std::ofstream file(jsonPath);
    if (file.is_open()) {
        file << metadata.dump(4);
        file.close();
        LOG("WorkshopDownloader: Created metadata at {}", jsonPath.string());
    } else {
        LOG("WorkshopDownloader: Failed to create metadata file");
    }
}

void WorkshopDownloader::RenameUdkToUpk(const std::filesystem::path& mapFolder)
{
    // Find .udk file and rename to .upk
    std::string udkFile = FindUdkInDirectory(mapFolder);
    if (!udkFile.empty()) {
        std::filesystem::path udkPath = mapFolder / udkFile;
        std::filesystem::path upkPath = mapFolder / (udkFile.substr(0, udkFile.length() - 4) + ".upk");
        
        try {
            std::filesystem::rename(udkPath, upkPath);
            LOG("WorkshopDownloader: Renamed {} to {}", udkFile, upkPath.filename().string());
        } catch (const std::exception& e) {
            LOG("WorkshopDownloader: Failed to rename .udk to .upk: {}", e.what());
        }
    }
}

std::string WorkshopDownloader::FindUdkInDirectory(const std::filesystem::path& dir)
{
    if (!std::filesystem::exists(dir)) {
        return "";
    }
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".udk") {
            return entry.path().filename().string();
        }
    }
    
    return "";
}

std::string WorkshopDownloader::SanitizeMapName(const std::string& name)
{
    std::string safe = name;
    
    // Replace spaces with underscores
    std::replace(safe.begin(), safe.end(), ' ', '_');
    
    // Remove special characters
    std::vector<std::string> specials = { "/", "\\", "?", ":", "*", "\"", "<", ">", "|", "-", "#" };
    for (const auto& special : specials) {
        EraseAll(safe, special);
    }
    
    return safe;
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

void WorkshopDownloader::EraseAll(std::string& str, const std::string& from)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.erase(pos, from.length());
    }
}
