#include "pch.h"
#include "WorkshopDownloader.h"
#include <fstream>
#include <sstream>
#include <algorithm>

WorkshopDownloader::WorkshopDownloader(std::shared_ptr<GameWrapper> gw) 
    : gameWrapper(gw)
{
    BakkesmodPath = gw->GetDataFolder().string() + "\\";
    IfNoPreviewImagePath = BakkesmodPath + "SuiteSpot\\Workshop\\NoPreview.jpg";
}

void WorkshopDownloader::GetResults(std::string keyWord, int IndexPage)
{
    RLMAPS_Searching = true;
    {
        std::lock_guard<std::mutex> lock(resultsMutex);
        RLMAPS_MapResultList.clear();
    }
    completedRequests = 0;
    RLMAPS_PageSelected = IndexPage;
    
    if (IndexPage == 1) {
        NumPages = 0;
        std::thread t2(&WorkshopDownloader::GetNumPages, this, keyWord);
        t2.detach();
    }

    std::string searchUrl = rlmaps_url + keyWord + "&page=" + std::to_string(IndexPage);
    
    CurlRequest req;
    req.url = searchUrl;
    
    HttpWrapper::SendCurlRequest(req, [this, keyWord](int code, std::string result) {
        LOG("Workshop search response code: {}, length: {}", code, result.length());
        if (code != 200) {
            LOG("Workshop search failed with HTTP code {}", code);
            RLMAPS_Searching = false;
            return;
        }

        try {
            nlohmann::json actualJson = nlohmann::json::parse(result);

            if (!actualJson.is_array()) {
                LOG("Workshop search response is not an array");
                RLMAPS_Searching = false;
                return;
            }

            RLMAPS_NumberOfMapsFound = (int)actualJson.size();
            LOG("Workshop search found {} maps", RLMAPS_NumberOfMapsFound);

            int expectedRequests = (int)actualJson.size();
            
            for (int index = 0; index < expectedRequests; ++index) {
                std::thread t2(&WorkshopDownloader::GetMapResult, this, actualJson, index);
                t2.detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Wait for all map requests to complete (success or failure)
            std::unique_lock<std::mutex> lock(resultsMutex);
            resultsCV.wait(lock, [this, expectedRequests]() {
                return completedRequests.load() >= expectedRequests;
            });

            RLMAPS_Searching = false;
        }
        catch (const std::exception& e) {
            LOG("Workshop search JSON parse error: {}", e.what());
            RLMAPS_Searching = false;
        }
    });
}

void WorkshopDownloader::GetMapResult(nlohmann::json maps, int index)
{
    try {
        RLMAPS_MapResult result;
        if (!maps[index].contains("id") || !maps[index].contains("name") || !maps[index].contains("description") || !maps[index].contains("namespace")) {
            LOG("Workshop map index {} missing required fields", index);
            completedRequests++;
            resultsCV.notify_one();
            return;
        }

        // Safely extract fields that might be numbers or strings
        auto& id_json = maps[index]["id"];
        result.ID = id_json.is_string() ? id_json.get<std::string>() : id_json.dump();
        
        // Remove quotes if dump() was used (though nlohmann::json::dump on numbers shouldn't have them)
        result.ID.erase(std::remove(result.ID.begin(), result.ID.end(), '\"'), result.ID.end());

        auto& name_json = maps[index]["name"];
        result.Name = name_json.is_string() ? name_json.get<std::string>() : name_json.dump();
        result.Name.erase(std::remove(result.Name.begin(), result.Name.end(), '\"'), result.Name.end());

        auto& desc_json = maps[index]["description"];
        result.Description = desc_json.is_string() ? desc_json.get<std::string>() : desc_json.dump();
        
        CleanHTML(result.Description);
        
        std::string releaseUrl = "https://celab.jetfox.ovh/api/v4/projects/" + result.ID + "/releases";
        
        CurlRequest releaseReq;
        releaseReq.url = releaseUrl;
        
        HttpWrapper::SendCurlRequest(releaseReq, [this, result, index, maps](int code, std::string responseText) mutable {
            // Ensure we always notify completion, even on failure
            auto notifyCompletion = [this]() {
                completedRequests++;
                resultsCV.notify_one();
            };
            
            if (code != 200) {
                LOG("Failed to get releases for map {}, code: {}", result.Name, code);
                notifyCompletion();
                return;
            }

            try {
                nlohmann::json releaseJson = nlohmann::json::parse(responseText);

                if (!releaseJson.is_array()) {
                    LOG("Release response is not an array for map {}", result.Name);
                    notifyCompletion();
                    return;
                }

                std::vector<RLMAPS_Release> releases;
                for (int release_index = 0; release_index < (int)releaseJson.size(); ++release_index) {
                    RLMAPS_Release release;
                    release.name = releaseJson[release_index]["name"].get<std::string>();
                    release.tag_name = releaseJson[release_index]["tag_name"].get<std::string>();
                    release.description = releaseJson[release_index]["description"].get<std::string>();

                    if (releaseJson[release_index]["assets"]["links"].size() > 0) {
                        release.pictureLink = releaseJson[release_index]["assets"]["links"][0]["url"].get<std::string>();
                    }
                    if (releaseJson[release_index]["assets"]["links"].size() > 1) {
                        release.downloadLink = releaseJson[release_index]["assets"]["links"][1]["url"].get<std::string>();

                        std::string zipNameUnsafe = releaseJson[release_index]["assets"]["links"][1]["name"].get<std::string>();
                        std::string specials[] = { "/", "\\", "?", ":", "*", "\"", "<", ">", "|", "#", "'", "`" };
                        for (auto special : specials) {
                            EraseAll(zipNameUnsafe, special);
                        }
                        release.zipName = zipNameUnsafe;
                    }

                    releases.push_back(release);
                }

                result.releases = releases;
                result.Size = "10000000";
                
                auto& author_json = maps[index]["namespace"]["path"];
                result.Author = author_json.is_string() ? author_json.get<std::string>() : author_json.dump();
                result.Author.erase(std::remove(result.Author.begin(), result.Author.end(), '\"'), result.Author.end());

                if (!releases.empty()) {
                    result.PreviewUrl = releases[0].pictureLink;
                }

                LOG("Workshop map: {}", result.Name);

                fs::path resultImagePath = BakkesmodPath + "SuiteSpot\\Workshop\\img\\" + result.ID + ".jfif";

                if (!DirectoryOrFileExists(resultImagePath)) {
                    result.IsDownloadingPreview = true;
                    int newIndex = -1;
                    {
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        RLMAPS_MapResultList.push_back(result);
                        newIndex = (int)RLMAPS_MapResultList.size() - 1;
                    }
                    notifyCompletion();
                    DownloadPreviewImage(result.PreviewUrl, resultImagePath.string(), newIndex);
                } else {
                    result.ImagePath = resultImagePath;
                    // result.Image = std::make_shared<ImageWrapper>(resultImagePath, false, true);
                    result.isImageLoaded = true;
                    {
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        RLMAPS_MapResultList.push_back(result);
                    }
                    notifyCompletion();
                }
            }
            catch (const std::exception& e) {
                LOG("Failed to parse releases for map {}: {}", result.Name, e.what());
                notifyCompletion();
            }
        });
    } catch (const std::exception& e) {
        LOG("CRITICAL: Error in GetMapResult thread: {}", e.what());
        completedRequests++;
        resultsCV.notify_one();
    }
}

void WorkshopDownloader::GetNumPages(std::string keyWord)
{
    int ResultsSize = 20;
    std::string searchUrl = rlmaps_url + keyWord;

    CurlRequest req;
    req.url = searchUrl;

    HttpWrapper::SendCurlRequest(req, [this, ResultsSize](int code, std::string result) {
        if (code != 200) return;

        try {
            nlohmann::json actualJson = nlohmann::json::parse(result);
            if (actualJson.is_array()) {
                NumPages = ((int)actualJson.size() / ResultsSize) + 1;
                LOG("Workshop search found {} pages", NumPages);
            }
        }
        catch (...) {
            // Ignore parse errors for page count
        }
    });
}

void WorkshopDownloader::RLMAPS_DownloadWorkshop(std::string folderpath, RLMAPS_MapResult mapResult, RLMAPS_Release release)
{
    std::string workshopSafeMapName = SanitizeMapName(mapResult.Name);
    
    std::string Workshop_Dl_Path;
    if (folderpath.back() == '/' || folderpath.back() == '\\') {
        Workshop_Dl_Path = folderpath + workshopSafeMapName;
    } else {
        Workshop_Dl_Path = folderpath + "/" + workshopSafeMapName;
    }
    
    try {
        fs::create_directory(Workshop_Dl_Path);
        LOG("Workshop directory created: {}", Workshop_Dl_Path);
    } catch (const std::exception& ex) {
        LOG("Failed to create directory: {}", ex.what());
        FolderErrorText = ex.what();
        FolderErrorBool = true;
        return;
    }
    
    CreateJSONLocalWorkshopInfos(workshopSafeMapName, Workshop_Dl_Path + "/", 
                                 mapResult.Name, mapResult.Author, 
                                 mapResult.Description, mapResult.PreviewUrl);
    LOG("JSON created: {}/{}.json", Workshop_Dl_Path, workshopSafeMapName);
    
    if (DirectoryOrFileExists(mapResult.ImagePath)) {
        fs::copy(mapResult.ImagePath, Workshop_Dl_Path + "/" + workshopSafeMapName + ".jfif");
        LOG("Preview pasted: {}/{}.jfif", Workshop_Dl_Path, workshopSafeMapName);
    }
    
    std::string download_url = release.downloadLink;
    LOG("Download URL: {}", download_url);
    std::string Folder_Path = Workshop_Dl_Path + "/" + release.zipName;
    
    RLMAPS_WorkshopDownload_Progress = 0;
    RLMAPS_Download_Progress = 0;
    RLMAPS_IsDownloadingWorkshop = true;
    
    LOG("Download starting...");
    
    CurlRequest req;
    req.url = download_url;
    req.progress_function = [this](double file_size, double downloaded, ...) {
        RLMAPS_Download_Progress = downloaded;
        RLMAPS_WorkshopDownload_FileSize = file_size;
    };
    
    HttpWrapper::SendCurlRequest(req, [this, Folder_Path, Workshop_Dl_Path](int code, char* data, size_t size) {
        if (code == 200) {
            std::ofstream out_file{ Folder_Path, std::ios_base::binary };
            if (out_file) {
                out_file.write(data, size);
                out_file.close();
                
                LOG("Workshop downloaded to: {}", Workshop_Dl_Path);
                
                ExtractZipPowerShell(Folder_Path, Workshop_Dl_Path);
                
                int checkTime = 0;
                while (UdkInDirectory(Workshop_Dl_Path) == "Null") {
                    LOG("Extracting zip file");
                    if (checkTime > 10) {
                        LOG("Failed extracting the map zip file");
                        RLMAPS_IsDownloadingWorkshop = false;
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    checkTime++;
                }
                
                LOG("File extracted");
                RenameFileToUPK(Workshop_Dl_Path);
                RLMAPS_IsDownloadingWorkshop = false;
            } else {
                LOG("Failed to open output file: {}", Folder_Path);
                RLMAPS_IsDownloadingWorkshop = false;
            }
        } else {
            LOG("Workshop download failed with code {}", code);
            RLMAPS_IsDownloadingWorkshop = false;
        }
    });
    
    // Monitor download progress on this thread
    while (RLMAPS_IsDownloadingWorkshop.load()) {
        LOG("downloading...............");
        RLMAPS_WorkshopDownload_Progress = RLMAPS_Download_Progress.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void WorkshopDownloader::DownloadPreviewImage(std::string downloadUrl, std::string filePath, int mapResultIndex)
{
    if (downloadUrl.empty()) {
        return;
    }
    
    fs::create_directories(fs::path(filePath).parent_path());
    
    CurlRequest req;
    req.url = downloadUrl;
    
    HttpWrapper::SendCurlRequest(req, [this, filePath, mapResultIndex](int code, char* data, size_t size) {
        if (code == 200) {
            try {
                std::ofstream outFile(filePath, std::ios::binary);
                if (outFile) {
                    outFile.write(data, size);
                    outFile.close();
                    
                    {
                        std::lock_guard<std::mutex> lock(resultsMutex);
                        if (mapResultIndex >= 0 && mapResultIndex < RLMAPS_MapResultList.size()) {
                            RLMAPS_MapResultList[mapResultIndex].ImagePath = filePath;
                            // Image loading deferred to Render thread to avoid crash
                            RLMAPS_MapResultList[mapResultIndex].IsDownloadingPreview = false;
                        }
                    }
                    
                    LOG("Preview downloaded: {}", filePath);
                }
            } catch (const std::exception& e) {
                LOG("Error writing preview file {}: {}", filePath, e.what());
            }
        }
    });
}

void WorkshopDownloader::CreateJSONLocalWorkshopInfos(std::string jsonFileName, std::string workshopMapPath,
                                                       std::string mapTitle, std::string mapAuthor,
                                                       std::string mapDescription, std::string mapPreviewUrl)
{
    std::string filename = workshopMapPath + jsonFileName + ".json";
    std::ofstream JSONFile(filename);
    
    nlohmann::json j;
    j["Title"] = mapTitle;
    j["Author"] = mapAuthor;
    j["Description"] = mapDescription;
    j["PreviewUrl"] = mapPreviewUrl;
    
    JSONFile << j.dump();
    JSONFile.close();
}

void WorkshopDownloader::ExtractZipPowerShell(std::string zipFilePath, std::string destinationPath)
{
    std::string extractCommand = "powershell.exe Expand-Archive -LiteralPath '" + 
                                 zipFilePath + "' -DestinationPath '" + 
                                 destinationPath + "' -Force";
    system(extractCommand.c_str());
}

void WorkshopDownloader::RenameFileToUPK(fs::path filePath)
{
    std::string udkFile = UdkInDirectory(filePath.string());
    
    if (udkFile != "Null") {
        fs::path udkPath = filePath / udkFile;
        std::string upkName = udkFile.substr(0, udkFile.length() - 4) + ".upk";
        fs::path upkPath = filePath / upkName;
        
        try {
            fs::rename(udkPath, upkPath);
            LOG("Renamed {} to {}", udkFile, upkName);
        } catch (const std::exception& e) {
            LOG("Failed to rename .udk to .upk: {}", e.what());
        }
    }
}

std::string WorkshopDownloader::UdkInDirectory(std::string dirPath)
{
    if (!DirectoryOrFileExists(fs::path(dirPath))) {
        return "Null";
    }
    
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".udk") {
                return entry.path().filename().string();
            }
        }
    }
    
    return "Null";
}

std::string WorkshopDownloader::SanitizeMapName(const std::string& name)
{
    std::string safe = name;
    ReplaceAll(safe, " ", "_");
    
    std::string specials[] = { "/", "\\", "?", ":", "*", "\"", "<", ">", "|", "-", "#" };
    for (auto special : specials) {
        EraseAll(safe, special);
    }
    
    return safe;
}

void WorkshopDownloader::CleanHTML(std::string& S)
{
    size_t pos = 0;
    while ((pos = S.find('<')) != std::string::npos) {
        size_t endPos = S.find('>', pos);
        if (endPos != std::string::npos) {
            S.erase(pos, endPos - pos + 1);
        } else {
            break;
        }
    }
}

void WorkshopDownloader::EraseAll(std::string& str, const std::string& from)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.erase(pos, from.length());
    }
}

void WorkshopDownloader::ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length();
    }
}

bool WorkshopDownloader::DirectoryOrFileExists(const fs::path& p)
{
    return fs::exists(p);
}
