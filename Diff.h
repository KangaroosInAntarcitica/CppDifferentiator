#ifndef FINAL_PROJECT_DIFF_H
#define FINAL_PROJECT_DIFF_H

#include "CppParser.h"
#include <unordered_map>

class FunctionDiffStorage;

class Diff {
protected:
    static const std::string DERIVATIVE_WRT_PREFIX;
    static const std::string DERIVATIVE_VAR_PREFIX;
    static const std::string DERIVATIVE_FILE_PREFIX;
    static const std::string DERIVATIVE_FUNCTION_PREFIX;

public:
    struct DiffContext {
        std::unordered_map<std::string, std::shared_ptr<Variable>> derivedVariables;
        std::unordered_map<std::string, std::shared_ptr<Variable>> arguments;
        std::vector<std::string> argumentNames;
        std::unordered_map<std::string, int> argumentIndexed;
        std::shared_ptr<Context> funcContext;
        std::shared_ptr<FunctionDiffStorage> functionDiffStorage;

        DiffContext(std::shared_ptr<Function> function, std::shared_ptr<FunctionDiffStorage> storage);
    };

    virtual std::string createDerivativeName(std::shared_ptr<Variable> variable, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt);

    virtual std::shared_ptr<Expression> diff(std::shared_ptr<Expression> expression, std::shared_ptr<DiffContext> context,
                                             std::shared_ptr<Variable> wrt, bool leftEquality=false, bool topStatement=false);
    virtual std::shared_ptr<Expression> diff(std::shared_ptr<Variable> variable, std::shared_ptr<DiffContext> context,
                                             std::shared_ptr<Variable> wrt, bool leftEquality=false, bool topStatement=false);
    virtual std::shared_ptr<Expression> diff(std::shared_ptr<ElementaryValue> value, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt);
    virtual std::shared_ptr<Expression> diff(std::shared_ptr<UnaryOperator> oper, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt);
    virtual std::shared_ptr<Expression> diff(std::shared_ptr<BinaryOperator> oper, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt, bool leftEquality=false);
    virtual std::shared_ptr<Expression> diff(std::shared_ptr<Call> call, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt);

    virtual std::vector<std::shared_ptr<Statement>> diff(std::shared_ptr<Statement> statement, std::shared_ptr<DiffContext> context, bool oneStatementRequired=false);
    virtual std::shared_ptr<BlockStatement> diff(std::shared_ptr<BlockStatement> block, std::shared_ptr<DiffContext> context);

    virtual std::shared_ptr<FunctionDeclaration> diff(std::shared_ptr<FunctionDeclaration> decl);
    virtual std::shared_ptr<Function> diff(std::shared_ptr<Function> function, std::shared_ptr<FunctionDiffStorage> storage);
    virtual std::shared_ptr<FileNode> diff(std::shared_ptr<FileNode> file, std::shared_ptr<FunctionDiffStorage> storage);

    virtual std::shared_ptr<Expression> simplify(std::shared_ptr<Expression> expression);

protected:
    virtual std::vector<std::shared_ptr<Expression>> getIndexesOfIndexedArg(std::shared_ptr<Expression> expression, std::string wrt);
    void getIndexesOfIndexedArg(std::shared_ptr<Expression> expression, std::string wrt, std::vector<std::shared_ptr<Expression>> &found);

public:
    static std::shared_ptr<FileNode> takeDiff(std::shared_ptr<FileNode> file, std::shared_ptr<FunctionDiffStorage> storage);
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
