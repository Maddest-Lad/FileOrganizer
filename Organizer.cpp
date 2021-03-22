// Organizer.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <filesystem>
#include <set>
#include <map>
namespace fs = std::filesystem;

// A C++ Script (Eventually I'll Make a GUI) Which Sorts Files Into Sub-Folders Based On File Extension


// Create A Map Of Extension Strings, Which Map To Sets Containing The Paths Of Those Files
void groupFilesByExtention(std::map<std::string, std::set <fs::path>>& FileByExt, const std::string path) {
    
    // TLDR For File In Path
    for (const fs::directory_entry entry : fs::directory_iterator(path)) {
        
        // Ignore Directories 
        if (entry.path().has_extension()) {
                
            // Is This Value Already in The Set, If So, Don't Create The File Map
            if (FileByExt.count(entry.path().extension().string()) < 0) {
                // There's Probably an Inline Way, I Just Don't Know It
                std::set <fs::path> files;
                FileByExt[entry.path().extension().string()] = files;
            }
            // Lastly, Add The Entry (path) to the Set
            FileByExt[entry.path().extension().string()].insert(entry);
        }
    }
}

void createFoldersAndMoveFiles(std::map<std::string, std::set <fs::path>> FileByExt, fs::path outputPath) {
    // Loop Through ExtMap
    for (std::pair<std::string, std::set <fs::path>> element : FileByExt) {
              
            // Grab The Current Extension and Create It's Folder
            fs::path ext = element.first;
            fs::path outputFolder = outputPath / ext;
            fs::create_directory(outputFolder);

            // Loop Through That Extention's File Set, And Move Them to outputFolder
            std::set <fs::path> pathAndFilename = element.second;

            for (fs::path file : element.second) {
                fs::path newPath = outputFolder / file.filename();
                fs::rename(file, newPath);
            }   
    }
}

int main()
{
    // Get The Input & Output Paths
    // TODO, Throw This All in A Try Catch Block For Validation And/Or Integrate Filesystem Hooks Directly
    std::string inputPath;
    std::string outputPath;
        
    std::cout << "Enter The Input Directory : ";
    std::getline(std::cin, inputPath);
        
    std::cout << "Enter The Output Directoryt : ";
    std::getline(std::cin, outputPath);


    // Generate & Populate The Extension Path Map
    std::map<std::string, std::set <fs::path>> ExtMap;
    groupFilesByExtention(ExtMap, inputPath);    

    // Using the ExtentionMap's Keys, Create The New Folders In outputPath, and Then Move The Files Over
    createFoldersAndMoveFiles(ExtMap, outputPath);
}
