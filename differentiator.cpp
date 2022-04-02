#include <iostream>
#include "CppParser.h"
#include "Diff.h"
#include "DefaultFunctionDiffStorage.h"

int main(int argc, char *argv[]) {
    std::cout << "Beginning parsing files" << std::endl;

    for (int i = 1; i < argc; ++i) {
        std::cout << "Parsing file '" + std::string(argv[i]) + "'" << std::endl;
        FileNode file = CppParser::parseFile(std::string("../") + argv[i]);
        // std::cout << "Parsed file: \n" << file.to_string() << std::endl;
        Diff diff;
        FileNode dFile = diff.diff(file, DefaultFunctionDiffStorage::singleton());
        std::cout << "Writing file '" + dFile.name + "'" << std::endl;
        CppParser::writeFile(dFile);
        // std::cout << "Diff file: \n" << diffFile.to_string() << std::endl;
    }

    return 0;
}


