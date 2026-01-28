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
    RLMAPS_MapResultList.clear();
    RLMAPS_PageSelected = IndexPage;
    
    if (IndexPage == 1) {
        NumPages = 0;
        std::thread t2(&WorkshopDownloader::GetNumPages, this, keyWord);
        t2.detach();
    }

    std::string searchUrl = rlmaps_url + keyWord + "&page=" + std::to_string(IndexPage);
    
    CurlRequest req;
    req.url = searchUrl;
    
    HttpWrapper::SendCurlJsonRequest(req, [this, keyWord](int code, nlohmann::json actualJson) {
        if (code != 200 || !actualJson.is_array()) {
            LOG("Workshop search failed with code {}", code);
            RLMAPS_Searching = false;
            return;
        }
        
        RLMAPS_NumberOfMapsFound = actualJson.size();
        
        for (int index = 0; index < actualJson.size(); ++index) {
            std::thread t2(&WorkshopDownloader::GetMapResult, this, actualJson, index);
            t2.detach();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        while (RLMAPS_MapResultList.size() != actualJson.size()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        RLMAPS_Searching = false;
    });
}

void WorkshopDownloader::GetMapResult(nlohmann::json maps, int index)
{
    RLMAPS_MapResult result;
    result.ID = maps[index]["id"].get<std::string>();
    result.Name = maps[index]["name"].get<std::string>();
    result.Description = maps[index]["description"].get<std::string>();
    
    CleanHTML(result.Description);
    
    std::string releaseUrl = "https://celab.jetfox.ovh/api/v4/projects/" + result.ID + "/releases";
    
    CurlRequest releaseReq;
    releaseReq.url = releaseUrl;
    
    HttpWrapper::SendCurlJsonRequest(releaseReq, [this, result, index, maps](int code, nlohmann::json releaseJson) mutable {
        if (code != 200 || !releaseJson.is_array()) {
            return;
        }
        
        std::vector<RLMAPS_Release> releases;
        for (int release_index = 0; release_index < releaseJson.size(); ++release_index) {
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
        result.Author = maps[index]["namespace"]["path"].get<std::string>();
        
        if (!releases.empty()) {
            result.PreviewUrl = releases[0].pictureLink;
        }
        
        LOG("Workshop map: {}", result.Name);
        
        fs::path resultImagePath = BakkesmodPath + "SuiteSpot\\Workshop\\img\\" + result.ID + ".jfif";
        
        if (!DirectoryOrFileExists(resultImagePath)) {
            result.IsDownloadingPreview = true;
            RLMAPS_MapResultList.push_back(result);
            DownloadPreviewImage(result.PreviewUrl, resultImagePath.string(), RLMAPS_MapResultList.size() - 1);
        } else {
            result.ImagePath = resultImagePath;
            result.Image = std::make_shared<ImageWrapper>(resultImagePath, false, true);
            result.isImageLoaded = true;
            RLMAPS_MapResultList.push_back(result);
        }
    });
}

void WorkshopDownloader::GetNumPages(std::string keyWord)
{
    int ResultsSize = 20;
    std::string searchUrl = rlmaps_url + keyWord;
    
    CurlRequest req;
    req.url = searchUrl;
    
    HttpWrapper::SendCurlJsonRequest(req, [this, ResultsSize](int code, nlohmann::json actualJson) {
        if (code == 200 && actualJson.is_array()) {
            NumPages = (actualJson.size() / ResultsSize) + 1;
            LOG("Workshop search found {} pages", NumPages);
        }
    });
}

void WorkshopDownloader::RLMAPS_DownloadWorkshop(std::string folderpath, RLMAPS_MapResult mapResult, RLMAPS_Release release)
{
    UserIsChoosingYESorNO = true;
    
    while (UserIsChoosingYESorNO) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!AcceptTheDownload) {
        return;
    }
    
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
                RLMAPS_IsDownloadingWorkshop = false;
                
                ExtractZipPowerShell(Folder_Path, Workshop_Dl_Path);
                
                int checkTime = 0;
                while (UdkInDirectory(Workshop_Dl_Path) == "Null") {
                    LOG("Extracting zip file");
                    if (checkTime > 10) {
                        LOG("Failed extracting the map zip file");
                        return;
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    checkTime++;
                }
                
                LOG("File extracted");
                RenameFileToUPK(Workshop_Dl_Path);
            }
        } else {
            LOG("Workshop download failed with code {}", code);
            RLMAPS_IsDownloadingWorkshop = false;
        }
    });
    
    while (RLMAPS_IsDownloadingWorkshop == true) {
        LOG("downloading...............");
        RLMAPS_WorkshopDownload_Progress = RLMAPS_Download_Progress;
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
            std::ofstream outFile(filePath, std::ios::binary);
            if (outFile) {
                outFile.write(data, size);
                outFile.close();
                
                if (mapResultIndex >= 0 && mapResultIndex < RLMAPS_MapResultList.size()) {
                    RLMAPS_MapResultList[mapResultIndex].ImagePath = filePath;
                    RLMAPS_MapResultList[mapResultIndex].Image = std::make_shared<ImageWrapper>(filePath, false, true);
                    RLMAPS_MapResultList[mapResultIndex].isImageLoaded = true;
                    RLMAPS_MapResultList[mapResultIndex].IsDownloadingPreview = false;
                }
                
                LOG("Preview downloaded: {}", filePath);
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
