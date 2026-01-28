#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/wrappers/http/HttpWrapper.h"
#include "MapList.h"
#include "logging.h"
#include "IMGUI/json.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>

namespace fs = std::filesystem;

struct RLMAPS_Release
{
    std::string name;
    std::string tag_name;
    std::string description;
    std::string zipName;
    std::string downloadLink;
    std::string pictureLink;
};

struct RLMAPS_MapResult
{
    std::string ID;
    std::string Name;
    std::string Size;
    std::string Description;
    std::string PreviewUrl;
    std::string Author;
    std::vector<RLMAPS_Release> releases;
    fs::path ImagePath;
    std::shared_ptr<ImageWrapper> Image;
    bool isImageLoaded = false;
    bool IsDownloadingPreview = false;
};

class WorkshopDownloader
{
public:
    WorkshopDownloader(std::shared_ptr<GameWrapper> gw);
    
    void GetResults(std::string keyWord, int IndexPage);
    void GetMapResult(nlohmann::json maps, int index);
    void GetNumPages(std::string keyWord);
    
    void RLMAPS_DownloadWorkshop(std::string folderpath, RLMAPS_MapResult mapResult, RLMAPS_Release release);
    void DownloadPreviewImage(std::string downloadUrl, std::string filePath, int mapResultIndex);
    
    void CreateJSONLocalWorkshopInfos(std::string jsonFileName, std::string workshopMapPath, 
                                     std::string mapTitle, std::string mapAuthor, 
                                     std::string mapDescription, std::string mapPreviewUrl);
    void ExtractZipPowerShell(std::string zipFilePath, std::string destinationPath);
    void RenameFileToUPK(fs::path filePath);
    std::string UdkInDirectory(std::string dirPath);
    
    bool RLMAPS_Searching = false;
    int RLMAPS_NumberOfMapsFound = 0;
    int NumPages = 0;
    int RLMAPS_PageSelected = 0;
    std::vector<RLMAPS_MapResult> RLMAPS_MapResultList;
    
    bool RLMAPS_IsDownloadingWorkshop = false;
    int RLMAPS_Download_Progress = 0;
    int RLMAPS_WorkshopDownload_Progress = 0;
    int RLMAPS_WorkshopDownload_FileSize = 0;
    
    bool UserIsChoosingYESorNO = false;
    bool AcceptTheDownload = false;
    bool FolderErrorBool = false;
    std::string FolderErrorText;
    
    std::string BakkesmodPath;
    std::string IfNoPreviewImagePath;
    std::string rlmaps_url = "https://celab.jetfox.ovh/api/v4/projects/?search=";
    
private:
    std::shared_ptr<GameWrapper> gameWrapper;
    
    std::string SanitizeMapName(const std::string& name);
    void CleanHTML(std::string& S);
    void EraseAll(std::string& str, const std::string& from);
    void ReplaceAll(std::string& str, const std::string& from, const std::string& to);
    bool DirectoryOrFileExists(const fs::path& p);
};
