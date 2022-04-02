#ifndef DIFFERENTIATOR_CPPPARSER_H
#define DIFFERENTIATOR_CPPPARSER_H

#include <list>
#include <string>
#include <map>
#include <memory>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include "SyntaxTreeNode.h"

class FileReader {
private:
    std::ifstream ifile;
    std::string filePath;
    std::string line;
    bool end = false;
    int lineN = 0;
    int charN = -1;
    char nextChar;

    static bool isNumber(char c);
    static bool isLowChar(char c);
    static bool isUpperChar(char c);
    static bool isWhitespace(char c);
    static bool isUnaryOperator(char c);
    static bool isOperator(char c);

    const char UNDERSCORE = '_';
    const char OPEN_ROUND = '(';
    const char CLOSE_ROUND = ')';
    const char OPEN_CURLY = '{';
    const char CLOSE_CURLY = '}';
    const char SEMI_COLON = ';';
    const char POINT = '.';
    const char COMMA = ',';
    const char COLON = ':';

    const std::string RETURN = "return";

public:
    explicit FileReader(std::string& filePath);
    void step();
    void stepBack(int nSteps);
    void skipWhitespace();
    void verifyNextCharIs(char c, bool skipSpace=true);
    std::string parseIdentifier(bool allowColon=false, bool skipSpace=true);
    std::string parseOperator();
    Expression* parseNumber();
    Statement* parseStatement(Context& context);
    BlockStatement *parseBlock(Context &context);
    Expression* parseExpression(Context& context);
    Expression* parseVariable(Context& context, bool declarationRequired=true);
    Expression* parseFunctionCall(Context &context, FunctionDeclaration *decl);
    FileStatement* parseFunction(Context& globalContext);
    Type parseType(Context& context);
    FileStatement *parseFileStatement(Context& context);
    FileNode parseFile();
};

class CppParser {
public:
    CppParser() = default;

    static FileNode parseFile(std::string filePath) {
        FileReader reader{filePath};
        FileNode file = reader.parseFile();
        return file;
    }

    static void writeFile(FileNode &file) {
        std::ofstream output;
        output.open(file.name);
        output << file.to_string();
        output.close();
    }
};

#endif //DIFFERENTIATOR_CPPPARSER_H
