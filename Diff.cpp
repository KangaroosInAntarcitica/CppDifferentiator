#include "Diff.h"
#include "FunctionDiffStorage.h"

const std::string Diff::DERIVATIVE_WRT_PREFIX = "d_";
const std::string Diff::DERIVATIVE_VAR_PREFIX = "_";
const std::string Diff::DERIVATIVE_FUNCTION_PREFIX = "d_";
const std::string Diff::DERIVATIVE_FILE_PREFIX = "d_";

Diff::DiffContext::DiffContext(Function *function, FunctionDiffStorage &storage):
        funcContext(function->context), functionDiffStorage(storage) {
    for (Variable *param: function->declaration->params) {
        arguments[param->name] = param;
        argumentNames.push_back(param->name);
    }
}

FileNode Diff::diff(FileNode &file, FunctionDiffStorage &storage) {
    FileNode dFile;
    dFile.context = file.context;

    size_t fileSeparator = file.name.find_last_of('/');
    std::string fileName, filePath;
    if (fileSeparator == std::string::npos) {
        fileName = file.name;
    } else {
        fileName = file.name.substr(fileSeparator + 1);
        filePath = file.name.substr(0, fileSeparator + 1);
    }

    dFile.name = filePath + DERIVATIVE_FILE_PREFIX + fileName;

    for (FileStatement *statement: file.statements) {
        switch (statement->getType()) {
            case FileStatement::FUNCTION:
                dFile.statements.push_back(diff(dynamic_cast<Function *>(statement), storage));
                break;
            case FileStatement::FUNCTION_DECLARATION:
                dFile.statements.push_back(diff(dynamic_cast<FunctionDeclaration *>(statement)));
                break;
            case FileStatement::INCLUDE:
                dFile.statements.push_back(statement);
                break;
            default:
                throw DiffException("File statement of this type not supported");
        }
    }

    return dFile;
}

FunctionDeclaration *Diff::diff(FunctionDeclaration *decl) {
    std::string name = DERIVATIVE_FUNCTION_PREFIX + decl->name;
    auto* dDecl = new FunctionDeclaration(name, decl->returnType);
    dDecl->params = decl->params;
    return dDecl;
}

Function* Diff::diff(Function* function, FunctionDiffStorage &storage) {
    DiffContext context{function, storage};
    auto* df = new Function(diff(function->declaration));
    df->context = function->context;
    df->block = diff(function->block, context);

    for (auto it=context.derivedVariables.begin(); it != context.derivedVariables.end(); ++it) {
        df->context.addVariable(it->second);
    }

    return df;
}

BlockStatement *Diff::diff(BlockStatement *block, DiffContext &context) {
    BlockStatement *dBlock = new BlockStatement();
    for (Statement *statement: block->statements) {
        std::vector<Statement *> dStatement = diff(statement, context);
        dBlock->statements.insert(dBlock->statements.end(), dStatement.begin(), dStatement.end());
    }
    return dBlock;
}

std::vector<Statement *> Diff::diff(Statement *statement, DiffContext &context, bool oneStatementRequired) {
    std::vector<Statement *> dStatements;
    if (statement->getType() == Statement::BLOCK) {
        dStatements.push_back(diff(dynamic_cast<BlockStatement *>(statement), context));
    } else if (statement->getType() == Statement::RETURN) {
        ReturnStatement *returnStatement = dynamic_cast<ReturnStatement *>(statement);
        for (std::string &argName: context.argumentNames) {
            dStatements.push_back(new ReturnStatement(diff(returnStatement->expr,
                                                           context, context.arguments[argName])));
        }
    } else if (statement->getType() == Statement::IF || statement->getType() == Statement::WHILE_LOOP) {
        auto *conditional = dynamic_cast<ConditionalStatement *>(statement);
        auto *dConditional = new ConditionalStatement(conditional->repeat, conditional->condition);
        dConditional->statement = diff(conditional->statement, context, true)[0];
        if (conditional->elseStatement != nullptr) {
            dConditional->elseStatement = diff(conditional->elseStatement, context, true)[0];
        }
        dStatements.push_back(dConditional);
    } else if (statement->getType() == Statement::ASSIGNMENT) {
        for (std::string &argName: context.argumentNames) {
            dStatements.push_back(diff(dynamic_cast<AssignmentStatement *>(statement),
                                       context, context.arguments[argName]));
        }
        dStatements.push_back(statement);
    } else {
        throw DiffException("Statement type differentiation not implemented");
    }

    if (oneStatementRequired && dStatements.size() > 1) {
        auto *block = new BlockStatement();
        block->statements = dStatements;
        dStatements = std::vector<Statement*>();
        dStatements.push_back(block);
    } else if (oneStatementRequired && dStatements.empty()){
        throw DiffException("At least one statement required");
    }

    return dStatements;
}

Statement* Diff::diff(AssignmentStatement* statement, DiffContext &context, Variable *wrt) {
    Expression *expr = statement->var;
    Expression *derivativeExpr;

    if (expr->getType() == Expression::ExpressionType::VARIABLE_DECLARATION) {
        auto *decl = dynamic_cast<VariableDeclaration *>(expr);
        std::string derName = createDerivativeName(decl->var, context, wrt);

        if (context.funcContext.definedVariables.count(derName)) {
            return nullptr;
        } else {
            Variable* variable = new Variable;
            variable->name = derName;
            variable->type = decl->var->type;
            VariableDeclaration *derivativeDecl = new VariableDeclaration;
            derivativeDecl->var = variable;
            derivativeExpr = derivativeDecl;
            context.derivedVariables[derName] = variable;
        }
    } else if (expr->getType() == Expression::ExpressionType::VARIABLE) {
        Variable *var = dynamic_cast<Variable *>(expr);
        std::string derivativeName = createDerivativeName(var, context, wrt);
        if (context.derivedVariables.count(derivativeName)) {
            derivativeExpr = context.derivedVariables[derivativeName];
        } else if (context.arguments.count(var->name)) {
            throw DiffException("Reassigning arguments is not allowed, as derivative will be changed");
        } else {
            throw DiffException("Derived variable '" + derivativeName + "' not found");
        }
    } else {
        throw DiffException("Left operand can only be a variable");
    }

    auto *derivativeStatement = new AssignmentStatement;
    derivativeStatement->var = derivativeExpr;

    switch (statement->op) {
        case AssignmentStatement::EQUALS:
        case AssignmentStatement::PLUS_EQUALS:
        case AssignmentStatement::MINUS_EQUALS:
            derivativeStatement->expr = diff(statement->expr, context, wrt);
            derivativeStatement->op = statement->op;
            break;
        default:
            throw DiffException("Assignment statement '" +
                AssignmentStatement::operatorToString(statement->op) + "' is not supported yet");
    }

    return derivativeStatement;
}

Expression *Diff::diff(Expression *expression, DiffContext &context, Variable *wrt) {
    switch(expression->getType()) {
        case Expression::VARIABLE:
            return diff(dynamic_cast<Variable *>(expression), context, wrt);
        case Expression::UNARY_OPERATOR:
            return diff(dynamic_cast<UnaryOperator *>(expression), context, wrt);
        case Expression::BINARY_OPERATOR:
            return diff(dynamic_cast<BinaryOperator *>(expression), context, wrt);
        case Expression::ELEMENTARY_VALUE:
            return diff(dynamic_cast<ElementaryValue *>(expression), context, wrt);
        case Expression::FUNCTION_CALL:
            return diff(dynamic_cast<FunctionCall *>(expression), context, wrt);
        default:
            throw DiffException("Unsupported Expression Type for differentiation provided");
    }
}

Expression* Diff::diff(Variable *variable, DiffContext &context, Variable *wrt) {
    std::string derName = createDerivativeName(variable, context, wrt);
    if (context.derivedVariables.count(derName)) {
        return context.derivedVariables[derName];
    } else if (variable == wrt) {
        return new Number("1");
    } else if (context.arguments.count(variable->name)) {
        return new Number("0");
    } else if (context.funcContext.isVariablePresent(derName)) {
        return context.funcContext.getVariable(derName);
    } else {
        throw DiffException("Cannot differentiate as variable '" + derName + "' was not defined");
    }
}

Expression* Diff::diff(UnaryOperator *oper, DiffContext &context, Variable *wrt) {
    switch(oper->op) {
        case UnaryOperator::Operation::PLUS:
        case UnaryOperator::Operation::MINUS:
        case UnaryOperator::Operation::BRACES:
            return new UnaryOperator(oper->op, diff(oper->expr, context, wrt));
        default:
            throw DiffException("Unsupported Unary Operator received");
    }
}

Expression* Diff::diff(BinaryOperator *oper, DiffContext &context, Variable *wrt) {
    BinaryOperator *top, *bottom;

    switch(oper->op) {
        case BinaryOperator::Operation::PLUS:
        case BinaryOperator::Operation::MINUS:
            return new BinaryOperator(oper->op,
                diff(oper->left, context, wrt),
                diff(oper->right, context, wrt));
        case BinaryOperator::Operation::MULTIPLY:
            return new BinaryOperator(BinaryOperator::Operation::PLUS,
                 new BinaryOperator(BinaryOperator::Operation::MULTIPLY, diff(oper->left, context, wrt), oper->right),
                 new BinaryOperator(BinaryOperator::Operation::MULTIPLY, oper->left, diff(oper->right, context, wrt)));
        case BinaryOperator::Operation::DIVIDE:
            top = new BinaryOperator(BinaryOperator::Operation::MINUS,
                new BinaryOperator(BinaryOperator::Operation::MULTIPLY, diff(oper->left, context, wrt), oper->right),
                new BinaryOperator(BinaryOperator::Operation::MULTIPLY, oper->left, diff(oper->right, context, wrt)));
            bottom = new BinaryOperator(BinaryOperator::Operation::MULTIPLY, oper->right, oper->right);
            return new BinaryOperator(BinaryOperator::Operation::DIVIDE, top, bottom);
        default:
            throw DiffException("Unsupported Binary Operator received");
    }
}

Expression* Diff::diff(ElementaryValue *value, DiffContext &context, Variable *wrt) {
    return new Number("0");
}

Expression* Diff::diff(FunctionCall *call, DiffContext &context, Variable *wrt) {
    Expression *result = context.functionDiffStorage.convert(call, *this, context, wrt);
    if (result == nullptr) {
        throw DiffException("Cannot differentiate function '" + call->to_string() +
            "' as cannot find function differentiation calculator");
    }
    return result;
}

std::string Diff::createDerivativeName(Variable *variable, DiffContext &context, Variable *wrt) {
    return DERIVATIVE_WRT_PREFIX + wrt->name + DERIVATIVE_VAR_PREFIX + variable->name;
}
