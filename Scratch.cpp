// I Can Similarly Iterate, When I Am Creating The Directories & Moving The Files
void printExtensionMap(std::map<std::string, std::set <fs::path>> FileByExt) {
    for (std::pair<std::string, std::set <fs::path>> element : FileByExt) {
        // Print Extension Type
        std::cout << element.first << " : " << std::endl;

        // Print All Files Of That Extension
        for (fs::path path : element.second) {
            std::cout << "    " << path << std::endl;
        }
        std::cout << std::endl;
    }
}