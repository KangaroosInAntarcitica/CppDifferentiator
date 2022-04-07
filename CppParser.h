#ifndef DIFFERENTIATOR_CPPPARSER_H
#define DIFFERENTIATOR_CPPPARSER_H

#include <list>
#include <map>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <string>
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
    static bool isIdentifierStart(char c);
    static bool isIdentifierMid(char c);

    static const char UNDERSCORE = '_';
    static const char OPEN_ROUND = '(';
    static const char CLOSE_ROUND = ')';
    static const char OPEN_CURLY = '{';
    static const char CLOSE_CURLY = '}';
    static const char OPEN_SQUARE = '[';
    static const char CLOSE_SQUARE = ']';
    static const char SEMI_COLON = ';';
    static const char POINT = '.';
    static const char COMMA = ',';
    static const char COLON = ':';

    const std::string RETURN = "return";

public:
    explicit FileReader(std::string& filePath);
    void step();
    void stepBack(int nSteps);
    void skipWhitespace();
    void verifyNextCharIs(char c, bool skipSpace=true);
    std::string parseIdentifier(bool allowColon=false, bool skipSpace=true);
    std::string parseOperator();
    Type parseType(std::shared_ptr<Context> context, std::string typeName="");
    std::shared_ptr<Expression> parseNumber();
    std::shared_ptr<Statement> parseStatement(std::shared_ptr<Context> context, bool functionStatement=true);
    std::shared_ptr<BlockStatement> parseBlock(std::shared_ptr<Context> context);
    std::shared_ptr<Expression> parseExpression(std::shared_ptr<Context> context, bool isFirst=false,
                                                bool missingAllowed=false, std::shared_ptr<Expression> left=nullptr);
    std::shared_ptr<Call> parseCall(std::shared_ptr<Context> context, std::string name="", std::shared_ptr<Variable> var=nullptr);
        std::shared_ptr<Variable> parseVariable(std::shared_ptr<Context> context, bool declarationRequired=true);
    std::shared_ptr<Statement> parseFileStatement(std::shared_ptr<Context> globalContext);
    std::shared_ptr<Statement> parseFunction(std::shared_ptr<Context> globalContext);
    std::shared_ptr<FileNode> parseFile(std::shared_ptr<Context> context);
};

class CppParser {
public:
    CppParser() = default;

    static std::shared_ptr<FileNode> parseFile(std::string filePath, std::shared_ptr<Context> context) {
        FileReader reader{filePath};
        return reader.parseFile(std::move(context));
    }

    static void writeFile(std::shared_ptr<FileNode> file) {
        std::ofstream output;
        output.open(file->name);
        output << file->to_string();
        output.close();
    }
};

#endif //DIFFERENTIATOR_CPPPARSER_H
