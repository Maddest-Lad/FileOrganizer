/*
 * The Model Component of the MVC Design Pattern
 * The Model, For All Intents and Purposes Doesn't Care About Displaying Data
 * Controller Will Have a Few Public Methods To Call That Can Change The Data
 * View Will Have Methods It Can Call to View The Current Data
 */

#include <iostream>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

/*
 * File Struct, Pairs Each File Path and Extension
 *
 */
const struct File {

    File(std::filesystem::path &path) : path(path), ext(path.extension().string().erase(0, 1)) {}

    const std::filesystem::path path;
    const std::string ext;
};


class OrganizerModel {

private:

    // Internal Trackers For Operations
    std::unordered_set<File> currentFiles; // Yet To Be Moved (Used For Displaying Inputs & Moving Themselves)
    std::unordered_set<File> failedFiles;  // Files That Couldn't Be Moves
    std::unordered_set<File> movedFiles;   // Mainly For The Output Display // Might Simplify Into A Method That Just Just Scans outputDirectory

    // Primary Directories
    fs::path inputDirectory;
    fs::path outputDirectory;

    // Member Variables
    int recursiveDepth;

    static int getDepth(const fs::path &a, const fs::path &b) {

        int aCnt = 0, bCnt = 0;

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

    OrganizerModel() : recursiveDepth(1) {}


    /*
     * Controller Methods
     * All fs::path Inputs Should Be Validated In Controller
     */

    // Set's The Input Folder and Add's It's Files To currentFiles
    void setInputDirectory(fs::path &path) {
        inputDirectory = path;

        for (auto i = fs::recursive_directory_iterator(inputDirectory); i != fs::recursive_directory_iterator(); ++i) {

            fs::path entry = i->path();

            if (getDepth(inputDirectory, entry) < recursiveDepth + 1 && entry.has_extension()) {
                currentFiles.insert(File(entry));
            }
        }
    }

    void setOutputDirectory(fs::path &path) {
        outputDirectory = path;
    }

    void moveFiles() {
        for (const File &file : currentFiles) {

            // Create The Output Folder
            fs::path outputFolder = outputDirectory / file.ext;
            fs::create_directory(outputFolder);

            // Try to Move the File And Then Add It To Moved/Failed Files Depending on It's Successful Movement
            try {
                fs::path newPath = outputFolder / file.path.filename();
                fs::rename(file.path, newPath);
                movedFiles.insert(file);
            } catch (fs::filesystem_error &error) {
                std::cout << error.what() << std::endl;
                failedFiles.insert(file);
            }
            currentFiles.erase(file);
        }
    }

    // Maybe Change currentFiles, movedFiles into a Stack for more than 1 undo operation
    void undo() {
        for (const File &file : movedFiles) {

            // Recreate Their New Location
            fs::path outputFolder = outputDirectory / file.ext;
            fs::path currentPath = outputFolder / file.path.filename();

            // Try to Move the File
            try {
                fs::rename(currentPath, file.path);
            } catch (fs::filesystem_error &error) {
                std::cout << error.what() << std::endl;
            }
        }
    }
};
