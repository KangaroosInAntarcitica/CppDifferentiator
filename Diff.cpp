#include "Diff.h"
#include "FunctionDiffStorage.h"

const std::string Diff::DERIVATIVE_WRT_PREFIX = "d_";
const std::string Diff::DERIVATIVE_VAR_PREFIX = "_";
const std::string Diff::DERIVATIVE_FUNCTION_PREFIX = "d_";
const std::string Diff::DERIVATIVE_FILE_PREFIX = "d_";

std::string Diff::createDerivativeName(std::shared_ptr<Variable> variable, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt) {
    return DERIVATIVE_WRT_PREFIX + wrt->name + DERIVATIVE_VAR_PREFIX + variable->name;
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<Expression> expression, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt, bool leftEquality) {
    if (leftEquality && expression->getType() != Expression::VARIABLE && expression->getType() != Expression::VARIABLE_DECLARATION) {
        throw DiffException("Only variables are allowed as assignable types in equalities");
    }

    switch(expression->getType()) {
        case Expression::VARIABLE:
        case Expression::VARIABLE_DECLARATION:
            return diff(std::dynamic_pointer_cast<Variable>(expression), context, wrt, leftEquality);
        case Expression::UNARY_OPERATOR:
            return diff(std::dynamic_pointer_cast<UnaryOperator>(expression), context, wrt);
        case Expression::BINARY_OPERATOR:
            return diff(std::dynamic_pointer_cast<BinaryOperator>(expression), context, wrt);
        case Expression::ELEMENTARY_VALUE:
            return diff(std::dynamic_pointer_cast<ElementaryValue>(expression), context, wrt);
        case Expression::CALL:
            return diff(std::dynamic_pointer_cast<Call>(expression), context, wrt);
        default:
            throw DiffException("Unsupported Expression Type for differentiation provided");
    }
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<ElementaryValue> value, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt) {
    return std::make_shared<Number>("0");
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<Variable> variable, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt, bool leftEquality) {
    std::string derName = createDerivativeName(variable, context, wrt);
    if (variable->declaration) {
        if (!leftEquality) {
            throw DiffException("Variable declaration is only allowed on the left of equalities");
        }

        if (context->derivedVariables.count(derName)) {
            return context->derivedVariables[derName];
        } else if (context->funcContext->variables.count(derName)) {
            return context->funcContext->variables[derName];
        } else {
            context->derivedVariables[derName] = std::make_shared<Variable>(variable->type, derName);
            return std::make_shared<Variable>(variable->type, derName, true);
        }
    }

    if (context->derivedVariables.count(derName)) {
        return context->derivedVariables[derName];
    } else if (context->funcContext->variables.count(derName)) {
        return context->funcContext->variables[derName];
    } else if (variable->name == wrt->name) {
        if (leftEquality) {
            context->derivedVariables[derName] = std::make_shared<Variable>(variable->type, derName);
            return std::make_shared<Variable>(variable->type, derName, true);
        }
        return std::make_shared<Number>("1");
    } else if (context->arguments.count(variable->name)) {
        if (leftEquality) {
            context->derivedVariables[derName] = std::make_shared<Variable>(variable->type, derName);
            return std::make_shared<Variable>(variable->type, derName, true);
        }
        return std::make_shared<Number>("0");
    } else {
        throw DiffException("Cannot differentiate as variable '" + derName + "' was not defined");
    }
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<UnaryOperator> oper, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt) {
    switch(oper->op) {
        case UnaryOperator::Operation::PLUS:
        case UnaryOperator::Operation::MINUS:
        case UnaryOperator::Operation::BRACES:
            return std::make_shared<UnaryOperator>(oper->op, diff(oper->expr, context, wrt));
        case UnaryOperator::Operation::PLUS_PLUS:
        case UnaryOperator::Operation::MINUS_MINUS:
            return diff(oper->expr, context, wrt);
        default:
            throw DiffException("Unsupported Unary Operator received");
    }
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<BinaryOperator> oper, std::shared_ptr<DiffContext> context,
                                       std::shared_ptr<Variable> wrt, bool leftEquality) {
    std::shared_ptr<BinaryOperator> left, right, combined;
    if (leftEquality && oper->op != BinaryOperator::INDEXING) {
        throw DiffException("Only Variables allowed on the left side of an assignment");
    }

    switch(oper->op) {
        case BinaryOperator::PLUS:
        case BinaryOperator::MINUS:
            return std::make_shared<BinaryOperator>(oper->op,
                                                    diff(oper->left, context, wrt),
                                                    diff(oper->right, context, wrt));
        case BinaryOperator::MULTIPLY:
        case BinaryOperator::MULTIPLY_EQUALS:
            left = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, diff(oper->left, context, wrt), oper->right);
            right = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, oper->left, diff(oper->right, context, wrt));
            combined = std::make_shared<BinaryOperator>(BinaryOperator::PLUS, left, right);
            if (oper->op == BinaryOperator::MULTIPLY_EQUALS) {
                return std::make_shared<BinaryOperator>(BinaryOperator::EQUALS, diff(oper->left, context, wrt, true), combined);
            }
            return combined;
        case BinaryOperator::DIVIDE:
        case BinaryOperator::DIVIDE_EQUALS:
            left = std::make_shared<BinaryOperator>(BinaryOperator::MINUS,
                                                   std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, diff(oper->left, context, wrt), oper->right),
                                                   std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, oper->left, diff(oper->right, context, wrt)));
            right = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, oper->right, oper->right);
            combined = std::make_shared<BinaryOperator>(BinaryOperator::DIVIDE, left, right);
            if (oper->op == BinaryOperator::DIVIDE_EQUALS) {
                return std::make_shared<BinaryOperator>(BinaryOperator::EQUALS, diff(oper->left, context, wrt, true), combined);
            }
            return combined;
        case BinaryOperator::EQUALS:
        case BinaryOperator::PLUS_EQUALS:
        case BinaryOperator::MINUS_EQUALS:
            return std::make_shared<BinaryOperator>(oper->op,
                                                    diff(oper->left, context, wrt, true),
                                                    diff(oper->right, context, wrt));
        case BinaryOperator::INDEXING:
            return std::make_shared<BinaryOperator>(BinaryOperator::INDEXING, diff(oper->left, context, wrt), oper->right);
        default:
            throw DiffException("Unsupported Binary Operator received");
    }
}

std::shared_ptr<Expression> Diff::diff(std::shared_ptr<Call> call, std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> result = context->functionDiffStorage->convert(call, *this, context, wrt);
    if (result == nullptr) {
        throw DiffException("Cannot differentiate function '" + call->to_string() +
                            "' as cannot find function differentiation calculator");
    }
    return result;
}

std::vector<std::shared_ptr<Statement> > Diff::diff(
        std::shared_ptr<Statement> statement, std::shared_ptr<DiffContext> context, bool oneStatementRequired) {
    std::vector<std::shared_ptr<Statement>> dStatements;
    if (statement->getType() == Statement::EXPRESSION) {
        for (std::string &argName: context->argumentNames) {
            std::shared_ptr<Variable> arg = context->arguments[argName];
            std::shared_ptr<ExpressionStatement> expressionStatement =
                    std::dynamic_pointer_cast<ExpressionStatement>(statement);
            std::shared_ptr<Expression> dExpr = diff(expressionStatement->expr, context, arg);
            if (dExpr != nullptr) {
                dStatements.push_back(std::make_shared<ExpressionStatement>(dExpr));
            }
        }
        dStatements.push_back(statement);
    } else if (statement->getType() == Statement::BLOCK) {
        dStatements.push_back(diff(std::dynamic_pointer_cast<BlockStatement>(statement), context));
    } else if (statement->getType() == Statement::RETURN) {
        std::shared_ptr<ReturnStatement> returnStatement = std::dynamic_pointer_cast<ReturnStatement>(statement);
        for (std::string &argName: context->argumentNames) {
            dStatements.push_back(std::make_shared<ReturnStatement>(diff(returnStatement->expr,
                                                                         context, context->arguments[argName])));
        }
    } else if (statement->getType() == Statement::IF || statement->getType() == Statement::WHILE_LOOP) {
        std::shared_ptr<ConditionalStatement> conditional = std::dynamic_pointer_cast<ConditionalStatement>(statement);

        std::shared_ptr<Statement> dStatement = diff(conditional->statement, context, true)[0];
        std::shared_ptr<Statement> dElseStatement;
        if (conditional->elseStatement != nullptr) {
            dElseStatement = diff(conditional->elseStatement, context, true)[0];
        }
        dStatements.push_back(std::make_shared<ConditionalStatement>(
                conditional->repeat, conditional->condition, dStatement, dElseStatement));
    } else if (statement->getType() == Statement::FOR_LOOP) {
        std::shared_ptr<ForLoop> forLoop = std::dynamic_pointer_cast<ForLoop>(statement);
        std::vector<std::shared_ptr<Statement>> dDefinition = diff(forLoop->definition, context);
        for (int i = 0; i < dDefinition.size() - 1; ++i) {
            dStatements.push_back(dDefinition[i]);
        }
        std::vector<std::shared_ptr<Statement>> dStatement = diff(forLoop->statement, context, true);
        dStatements.push_back(std::make_shared<ForLoop>(dDefinition[dDefinition.size() - 1], forLoop->condition,
                                                        forLoop->expr, dStatement[0]));
    } else if (statement->getType() == Statement::COMMENT) {
        dStatements.push_back(statement);
    } else {
        throw DiffException("Statement type differentiation not implemented");
    }

    if (oneStatementRequired && dStatements.size() > 1) {
        std::shared_ptr<BlockStatement> block = std::make_shared<BlockStatement>(dStatements);
        dStatements = std::vector<std::shared_ptr<Statement>>();
        dStatements.push_back(block);
    } else if (oneStatementRequired && dStatements.empty()){
        throw DiffException("At least one statement required");
    }

    return dStatements;
}

std::shared_ptr<BlockStatement> Diff::diff(std::shared_ptr<BlockStatement> block, std::shared_ptr<DiffContext> context) {
    std::vector<std::shared_ptr<Statement>> dStatements;
    for (std::shared_ptr<Statement> statement: block->statements) {
        std::vector<std::shared_ptr<Statement> > dStatement = diff(statement, context);
        dStatements.insert(dStatements.end(), dStatement.begin(), dStatement.end());
    }
    return std::make_shared<BlockStatement>(dStatements);
}

std::shared_ptr<FunctionDeclaration> Diff::diff(std::shared_ptr<FunctionDeclaration> decl) {
    std::string name = DERIVATIVE_FUNCTION_PREFIX + decl->name;
    return std::make_shared<FunctionDeclaration>(name, decl->returnType, decl->params);
}

std::shared_ptr<Function> Diff::diff(std::shared_ptr<Function> function, std::shared_ptr<FunctionDiffStorage> storage) {
    std::shared_ptr<DiffContext> context = std::make_shared<DiffContext>(function, storage);

    for (auto it=context->derivedVariables.begin(); it != context->derivedVariables.end(); ++it) {
        context->funcContext->addVariable(it->first, it->second);
    }

    return std::make_shared<Function>(context->funcContext, diff(function->declaration), diff(function->block, context));
}

Diff::DiffContext::DiffContext(std::shared_ptr<Function> function, std::shared_ptr<FunctionDiffStorage> storage):
        functionDiffStorage(storage) {
    funcContext = std::shared_ptr<Context>(function->context->copy());

    for (std::shared_ptr<Variable> param: function->declaration->params) {
        arguments[param->name] = param;
        argumentNames.push_back(param->name);
    }
}

std::shared_ptr<FileNode> Diff::diff(std::shared_ptr<FileNode> file, std::shared_ptr<FunctionDiffStorage> storage) {
    std::shared_ptr<Context> dContext = std::shared_ptr<Context>(file->context->copy());

    size_t fileSeparator = file->name.find_last_of('/');
    std::string fileName, filePath;
    if (fileSeparator == std::string::npos) {
        fileName = file->name;
    } else {
        fileName = file->name.substr(fileSeparator + 1);
        filePath = file->name.substr(0, fileSeparator + 1);
    }
    std::string dName = filePath + DERIVATIVE_FILE_PREFIX + fileName;

    std::vector<std::shared_ptr<Statement>> dStatements;
    for (std::shared_ptr<Statement> statement: file->statements) {
        switch (statement->getType()) {
            case Statement::FUNCTION:
                dStatements.push_back(diff(std::dynamic_pointer_cast<Function>(statement), storage));
                break;
            case Statement::FUNCTION_DECLARATION:
                dStatements.push_back(diff(std::dynamic_pointer_cast<FunctionDeclaration>(statement)));
                break;
            case Statement::INCLUDE:
            case Statement::COMMENT:
                dStatements.push_back(statement);
                break;
            default:
                throw DiffException("File statement of this type not supported");
        }
    }

    return std::make_shared<FileNode>(dContext, dName, dStatements);
}

std::shared_ptr<FileNode> Diff::takeDiff(std::shared_ptr<FileNode> file, std::shared_ptr<FunctionDiffStorage> storage) {
    std::shared_ptr<Diff> diff = std::make_shared<Diff>();
    return diff->diff(std::move(file), std::move(storage));
}

/*
std::shared_ptr<Statement> Diff::diff(std::shared_ptr<ExpressionStatement> statement,
                                      std::shared_ptr<DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> expr = statement->var;
    std::shared_ptr<Expression> derivativeExpr;

    if (expr->getType() == Expression::ExpressionType::VARIABLE_DECLARATION) {
        std::shared_ptr<auto> decl = std::dynamic_pointer_cast<VariableDeclaration>(expr);
        std::string derName = createDerivativeName(decl->var, context, wrt);

        if (context.funcContext.definedVariables.count(derName)) {
            return nullptr;
        } else {
            std::shared_ptr<Variable> variable = std::make_shared<Variable;>
            variable->name = derName;
            variable->type = decl->var->type;
            std::shared_ptr<VariableDeclaration> derivativeDecl = std::make_shared<VariableDeclaration;>
            derivativeDecl->var = variable;
            derivativeExpr = derivativeDecl;
            context.derivedVariables[derName] = variable;
        }
    } else if (expr->getType() == Expression::ExpressionType::VARIABLE) {
        std::shared_ptr<Variable> var = std::dynamic_pointer_cast<Variable>(expr);
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

    std::shared_ptr<auto> derivativeStatement = std::make_shared<AssignmentStatement;>
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
*/
