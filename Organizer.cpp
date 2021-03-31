//Organizer.cpp

#include <iostream>
#include <filesystem>
#include <set> //TODO Look into <unorganized_set> and <vector> if performance is lacking
#include <map>

namespace fs = std::filesystem;

// Struct Which Wraps the FileSystem Error Class, To Assist With Handling Errors That Might Crop Up
const struct customError {
    const bool error;
    const std::string errorMsg;
    
    customError(bool error, std::string errorMsg) : error(error), errorMsg(errorMsg) {}

};

class Organizer {

private:
    fs::path inputPath;
    fs::path outputPath;

    std::set <fs::path> currentFiles;
    std::set <fs::path> currentDirs; // Handle Cases Where We Only Want Specific Recursive Depth

    // What Extensions We Can't Move 
    // From a Technical Standpoint, .LNK (ie shortcuts) Cause Problems and Should be Automatically Excluded
    // From a User Standpoint, It'd be Nice To Choose Certain File Types To Ignore
    std::map<std::string, bool> blackListedExtensions;

    int recursiveDepth;

    // Only For Unpacking Folders
    bool deleteOriginal;
    bool ignoreExistingDirs;

    // Helper Method
    static int getDepth(const fs::path& a, const fs::path& b) {

        int aCnt = 0;
        int bCnt = 0;

        const std::string aStr(a.string());
        for (const char c : aStr) {
            if (c == '\\') {
                aCnt++;
            }
        }

        const std::string bStr(b.string());
        for (const char c : bStr) {
            if (c == '\\') {
                bCnt++;
            }
        }

        return abs(aCnt - bCnt);
    }

public:

    // TODO Figure Out More Of Big Picture
    // * Like Do Settings Get Saved From The GUI Version Each Time
    // * Maybe have this load a config.txt / settings.json style thing
    // Maybe Make This A Pseduo-Singleton Via Statics 
    // * (Although It Should Be Pretty Apparent That You Don't Won't Concurrent IO Operations)
    Organizer() :
        ignoreExistingDirs(false),
        deleteOriginal(false),
        recursiveDepth(1) 
    {
    }

    // Now This Might be Overkill For The Setter Methods,
    // But I'm Hoping This Prevents Any Path Based Errors Down The Line
    customError setInputPath(const std::string& str) {
        try {
            if (fs::exists(fs::path(str))) {
                inputPath = fs::path(str);
            }
        }

        catch (fs::filesystem_error fsErr) {
            return customError(true, fsErr.what());
        }

        return customError(false, "");
    }

    customError setOutputPath(std::string str) {
        try {
            if (fs::exists(fs::path(str))) {
                outputPath = fs::path(str);
            }
        }

        catch (fs::filesystem_error fsErr) {
            return customError(true, fsErr.what());
        }

        return customError(false, "");
    }

    // While There's No Recursive Depth Limit, We Should Warn The User About High Values
    void setRecursiveDepth(unsigned int i) {
        recursiveDepth = i;
    }

    // While Not Useful For This Class, This Should Be Used For UI Hooks
    std::set <fs::path> getCurrentFiles() {
        return currentFiles;
    }

    std::set <fs::path> getCurrentDirectories() {
        return currentDirs;
    }

    // This Should Not Be Run-able Until setInputPath() Has Succeeded At Least Once
    // Otherwise, It Just Iterates Through inputPath and Inserts to currentFiles & currentDirs
    // TODO Decide How to Delete currentFiles & currentDirs Contents If A New Path is Set
    // * Also Consider Splitting Different Input Paths Into Their Own Maps To Handle Multiple Input Folders At Once
    void loadInput() {

        for (auto i = fs::recursive_directory_iterator(inputPath); i != fs::recursive_directory_iterator(); ++i) {

            fs::path entry = i->path();

            // Keep Track of Dirs Out Of Our Scope In Case We Need To Move Them For Unpacking Funcitonality
            if (getDepth(inputPath, entry) > recursiveDepth-1 && !entry.has_extension()) {
                currentDirs.insert(entry);
                continue;
            }

            if (getDepth(inputPath, entry) < recursiveDepth+1 && entry.has_extension()) {
                currentFiles.insert(entry);
            }
        }
    }

    // STL predicate for case-insensitive collections.  Since NTFS is case-preserving but case-insensitive any
    // collection that stores paths should search in a case-insensitive way.
    struct CaseInsensitive {
        bool operator() (const std::string& lhs, const std::string& rhs) const {
            return _stricmp(lhs.c_str(), rhs.c_str()) < 0;
        }
    };

    // Create A Map Of Extension Strings That Map To Sets Containing The Paths Of Those Files
    // Implicit Argument of "this."currentFiles
    std::map <std::string, std::set<fs::path>, CaseInsensitive> generateExtensionMap() {

        std::map <std::string, std::set<fs::path>, CaseInsensitive> ExtensionMap;

        for (const fs::path entry : currentFiles) {

            std::string extStr(entry.extension().string().erase(0, 1)); // Trim the "." From The Extension (For Ease of Searching In a POSIX Filesystem)

            // Is This Value Already in The Set, If So, Don't Create The File Map
            if (ExtensionMap.find(extStr) != ExtensionMap.end()) {
                // There's Probably an Inline Way, I Just Don't Know It
                ExtensionMap[extStr] = std::set <fs::path>();
            }
            // Lastly, Add The Entry (path) to the Set
            ExtensionMap[extStr].insert(entry);
        }

        return ExtensionMap;
    }

    // Loop Through The < File Extension -> fs::path> Map and The Move the Files To Their Filetype Directory in OutputPath
    void sort() {

        // Loop Through the Extension Map
        auto extensionMap = generateExtensionMap();
        for (std::pair <std::string, std::set<fs::path>> element : extensionMap) {

            // Grab The Current Extension and Create It's Folder
            fs::path ext = element.first;
            fs::path outputFolder = outputPath / ext;
            
            // TODO: Error handling
            fs::create_directory(outputFolder);

            // Loop Through That Extension's File Set, And Move Them to outputFolder
            std::set <fs::path>& pathAndFilename = element.second;

            for (fs::path file : pathAndFilename) {
                fs::path newPath = outputFolder / file.filename();
                fs::rename(file, newPath);
            }
        }
    }

    // The Second Main Feature, Unpack Files and Folders in inputPath, and Move The Files To outputPath
    // If recursiveDepth Would be Exceeded, Just Move The Parent Folder Instead
    // Likely, The End Result Should Be To Delete The Original Folders
    // TODO Fix Problem With Recursive Depth, Too Tired To Now
    customError unpackFiles() {
        try {
            // Loop Through the Files
            for (fs::path entry : currentFiles) {
                fs::path newPath = outputPath / entry.filename();
                fs::rename(entry, newPath);
            }

            std::cout << std::endl;

            // Loop Through the Directories
            for (fs::path entry : currentDirs) {

                fs::path diff = entry.lexically_relative(inputPath).filename(); // Gets The Difference Between 2 File Paths, then Extract The Topmost Dir
                fs::path newPath = outputPath / diff;

                std::cout << outputPath << " + " << diff << " = " << newPath << std::endl;
                fs::rename(entry, newPath);
            }
        }

        catch (fs::filesystem_error fsErr) {
            return customError(true, fsErr.what());
        }

        return customError(false, "");
    }
};

// Testing
int main() {

    Organizer o;

    o.setRecursiveDepth(3);

    o.setInputPath("C:\\Users\\Sam Pc\\Desktop\\in");
    o.setOutputPath("C:\\Users\\Sam Pc\\Desktop\\out");

    o.loadInput();

    std::set <fs::path> files = o.getCurrentFiles();
    std::set <fs::path> dirs = o.getCurrentDirectories();

    for (const fs::path file : files) {
        std::cout << file << std::endl;
    }

    for (const fs::path dir : dirs) {
        std::cout << dir << std::endl;
    }

    o.loadInput();
    o.unpackFiles();
    //o.sort();

}