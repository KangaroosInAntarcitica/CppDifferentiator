#ifndef FINAL_PROJECT_DIFF_H
#define FINAL_PROJECT_DIFF_H

#include "CppParser.h"
#include <unordered_map>

class FunctionDiffStorage;

class Diff {
private:
    static const std::string DERIVATIVE_WRT_PREFIX;
    static const std::string DERIVATIVE_VAR_PREFIX;
    static const std::string DERIVATIVE_FILE_PREFIX;
    static const std::string DERIVATIVE_FUNCTION_PREFIX;

public:
    struct DiffContext {
        std::unordered_map<std::string, Variable *> derivedVariables;
        std::unordered_map<std::string, Variable *> arguments;
        std::vector<std::string> argumentNames;
        Context &funcContext;
        FunctionDiffStorage &functionDiffStorage;

        DiffContext(Function *function, FunctionDiffStorage &storage);
    };

    virtual FileNode diff(FileNode &file, FunctionDiffStorage &storage);
    virtual Function* diff(Function *function, FunctionDiffStorage &storage);
    virtual FunctionDeclaration* diff(FunctionDeclaration *decl);
    virtual BlockStatement *diff(BlockStatement *block, DiffContext &context);
    virtual std::vector<Statement *> diff(Statement *statement, DiffContext &context, bool oneStatementRequired=false);
    virtual Statement* diff(AssignmentStatement *statement, DiffContext &context, Variable *wrt);
    virtual Expression* diff(Expression *expression, DiffContext &context, Variable *wrt);
    virtual Expression* diff(Variable *variable, DiffContext &context, Variable *wrt);
    virtual Expression* diff(UnaryOperator *oper, DiffContext &context, Variable *wrt);
    virtual Expression* diff(BinaryOperator *oper, DiffContext &context, Variable *wrt);
    virtual Expression* diff(ElementaryValue *value, DiffContext &context, Variable *wrt);
    virtual Expression* diff(FunctionCall *call, DiffContext &context, Variable *wrt);
    virtual std::string createDerivativeName(Variable *variable, DiffContext &context, Variable *wrt);
};

class DiffException : public std::exception
{
public:
    std::string message;
    DiffException(std::string message): message(message) {}

    const char *what() const noexcept override {
        return message.c_str();
    }
};

#endif //FINAL_PROJECT_DIFF_H
