#include <iostream>
#include "CppParser.h"
#include "Diff.h"
#include "DefaultFunctionDiffStorage.h"
#include <memory>

class DefaultContext: public Context {
public:
    DefaultContext() {
        Type integer("int");
        Type floating("float");
        Type doubleFloating("double");
        Type vector("std::vector");
        addType(integer.name, integer);
        addType(floating.name, floating);
        addType(doubleFloating.name, doubleFloating);
        addType(vector.name, vector);

        addFunction(FunctionSignature("std::cos", Type()));
        addFunction(FunctionSignature("std::sin", Type()));
        addFunction(FunctionSignature("std::pow", Type(), Type()));
        addFunction(FunctionSignature("std::log", Type()));
        addFunction(FunctionSignature("std::vector::size"));
    }
};

int main(int argc, char *argv[]) {
    std::cout << "Beginning parsing files" << std::endl;

    std::shared_ptr<Context> defaultContext = std::make_shared<DefaultContext>();

    for (int i = 1; i < argc; ++i) {
        std::cout << "Parsing file '" + std::string(argv[i]) + "'" << std::endl;
        std::shared_ptr<FileNode> file = CppParser::parseFile(std::string("../") + argv[i], defaultContext);
        std::cout << "Parsed file: \n" << file->to_string() << std::endl;
        std::shared_ptr<FunctionDiffStorage> diffStorage =
                std::make_shared<DefaultFunctionDiffStorage>(defaultContext);
        std::shared_ptr<FileNode> dFile = Diff::takeDiff(file, diffStorage);
        std::cout << "Writing file '" + dFile->name + "'" << std::endl;
        CppParser::writeFile(dFile);
//         std::cout << "Diff file: \n" << diffFile.to_string() << std::endl;
    }

    return 0;
}


