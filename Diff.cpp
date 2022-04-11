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
                                       std::shared_ptr<Variable> wrt, bool leftEquality, bool topStatement) {
    if (leftEquality && expression->getType() != Expression::VARIABLE &&
            expression->getType() != Expression::VARIABLE_DECLARATION &&
            expression->getType() != Expression::BINARY_OPERATOR) {
        throw DiffException("Only variables are allowed as assignable types in equalities");
    }

    switch(expression->getType()) {
        case Expression::VARIABLE:
        case Expression::VARIABLE_DECLARATION:
            return diff(std::dynamic_pointer_cast<Variable>(expression), context, wrt, leftEquality, topStatement);
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
                                       std::shared_ptr<Variable> wrt, bool leftEquality, bool topStatement) {
    std::string derName = createDerivativeName(variable, context, wrt);
    if (variable->declaration) {
        if (!leftEquality && !topStatement) {
            throw DiffException("Variable declaration is only allowed on the left of equalities");
        }

        if (context->derivedVariables.count(derName)) {
            return context->derivedVariables[derName];
        } else if (context->funcContext->variables.count(derName)) {
            return context->funcContext->variables[derName];
        } else {
            context->derivedVariables[derName] = std::make_shared<Variable>(variable->type, derName);
            std::shared_ptr<Call> constructorCall;
            if (variable->constructorCall != nullptr) {
                constructorCall = std::dynamic_pointer_cast<Call>(diff(variable->constructorCall, context, wrt));
            }
            return std::make_shared<Variable>(variable->type, derName, true, constructorCall);
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
            std::shared_ptr<Expression> dExpr = diff(expressionStatement->expr, context, arg, false, true);
            dExpr = simplify(dExpr);
            if (dExpr != nullptr) {
                dStatements.push_back(std::make_shared<ExpressionStatement>(dExpr));
            }
        }
        dStatements.push_back(statement);
    } else if (statement->getType() == Statement::BLOCK) {
        dStatements.push_back(diff(std::dynamic_pointer_cast<BlockStatement>(statement), context));
    } else if (statement->getType() == Statement::RETURN) {
        std::shared_ptr<ReturnStatement> returnStatement = std::dynamic_pointer_cast<ReturnStatement>(statement);
        if (context->argumentNames.size() == 1) {
            std::shared_ptr<Expression> expr = diff(returnStatement->expr, context, context->arguments[context->argumentNames[0]]);
            expr = simplify(expr);
            dStatements.push_back(std::make_shared<ReturnStatement>(expr));
        } else {
            std::shared_ptr<Variable> var = std::dynamic_pointer_cast<Variable>(returnStatement->expr);
            if (var == nullptr) {
                throw DiffException("Only variables are allowed as return types for functions with multiple arguments");
            }

            size_t nArgs = context->argumentNames.size();
            std::string returnName = DERIVATIVE_VAR_PREFIX + "return";
            Type returnType = Type("std::array", std::vector<Type>{var->type, Type(std::to_string(nArgs))});
            std::shared_ptr<Variable> returnVariable = std::make_shared<Variable>(returnType, returnName);
            context->funcContext->addVariable(returnName, returnVariable);
            context->derivedVariables[returnName] = returnVariable;
            dStatements.push_back(std::make_shared<ExpressionStatement>(std::make_shared<Variable>(returnType, returnName, true)));

            for (int i = 0; i < nArgs; ++i) {
                std::shared_ptr<Expression> left = std::make_shared<BinaryOperator>(BinaryOperator::INDEXING,
                     returnVariable, std::make_shared<Number>(i));
                std::shared_ptr<Expression> right = diff(returnStatement->expr, context, context->arguments[context->argumentNames[i]]);
                right = simplify(right);
                dStatements.push_back(std::make_shared<ExpressionStatement>(
                        std::make_shared<BinaryOperator>(BinaryOperator::EQUALS, left, right)));
            }
            dStatements.push_back(std::make_shared<ReturnStatement>(returnVariable));
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
    Type returnType = decl->returnType;
    if (decl->params.size() > 1) {
        returnType = Type("std::array", std::vector<Type>{decl->returnType, Type(std::to_string(decl->params.size()))});
    }
    return std::make_shared<FunctionDeclaration>(name, returnType, decl->params);
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
        argumentNames.push_back(param->name);
        if (param->type.name == "std::vector" || param->type.name == "std::array") {
            argumentIndexed[param->name] = 1;
        } else {
            argumentIndexed[param->name] = 0;
        }
        arguments[param->name] = param;
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
    dStatements.push_back(std::make_shared<Include>("array", true));

    for (std::shared_ptr<Statement> statement: file->statements) {
        switch (statement->getType()) {
            case Statement::FUNCTION:
                dStatements.push_back(diff(std::dynamic_pointer_cast<Function>(statement), storage));
                break;
            case Statement::FUNCTION_DECLARATION:
                dStatements.push_back(diff(std::dynamic_pointer_cast<FunctionDeclaration>(statement)));
                break;
            case Statement::INCLUDE:
                if (std::dynamic_pointer_cast<Include>(statement)->name == "array") break;
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

std::shared_ptr<Expression> Diff::simplify(std::shared_ptr<Expression> expression) {
    if (expression->getType() == Expression::UNARY_OPERATOR) {
        std::shared_ptr<UnaryOperator> op = std::dynamic_pointer_cast<UnaryOperator>(expression);
        std::shared_ptr<Expression> expr = simplify(op->expr);

        if (op->op == UnaryOperator::PLUS) {
            return expr;
        }

        return std::make_shared<UnaryOperator>(op->op, expr);
    } else if (expression->getType() == Expression::BINARY_OPERATOR) {
        std::shared_ptr<BinaryOperator> op = std::dynamic_pointer_cast<BinaryOperator>(expression);
        std::shared_ptr<Expression> left = simplify(op->left);
        std::shared_ptr<Expression> right = simplify(op->right);
        std::shared_ptr<Number> leftNumber = std::dynamic_pointer_cast<Number>(left);
        std::shared_ptr<Number> rightNumber = std::dynamic_pointer_cast<Number>(right);
        bool leftZero = leftNumber != nullptr && leftNumber->isZero();
        bool rightZero = rightNumber != nullptr && rightNumber->isZero();
        bool leftOne = leftNumber != nullptr && leftNumber->isOne();
        bool rightOne = rightNumber != nullptr && rightNumber->isOne();

        if (op->op == BinaryOperator::PLUS || op->op == BinaryOperator::MINUS) {
            if (leftZero) {
                return right;
            } else if (rightZero) {
                return left;
            } else if (leftNumber != nullptr && rightNumber != nullptr) {
                double value = op->op == BinaryOperator::PLUS ? leftNumber->value + rightNumber->value
                        : leftNumber->value - rightNumber->value;
                return std::make_shared<Number>(value);
            }
        } else if (op->op == BinaryOperator::MULTIPLY) {
            if (leftZero || rightZero) {
                return std::make_shared<Number>(0);
            } else if (leftOne) {
                return right;
            } else if (rightOne) {
                return left;
            } else if (leftNumber != nullptr && rightNumber != nullptr) {
                return std::make_shared<Number>(leftNumber->value * rightNumber->value);
            }
        } else if(op->op == BinaryOperator::DIVIDE) {
            if (rightOne) {
                return left;
            }
        }

        return std::make_shared<BinaryOperator>(op->op, left, right);
    } else if (expression->getType() == Expression::VARIABLE_DECLARATION) {
        std::shared_ptr<Variable> var = std::dynamic_pointer_cast<Variable>(expression);

        if (var->constructorCall) {
            return std::make_shared<Variable>(var->type, var->name, var->declaration,
                                              std::dynamic_pointer_cast<Call>(simplify(var->constructorCall)));
        }
    } else if (expression->getType() == Expression::CALL) {
        std::shared_ptr<Call> call = std::dynamic_pointer_cast<Call>(expression);

        if (call->args.empty()) {
            return call;
        }

        std::vector<std::shared_ptr<Expression>> args;
        for (std::shared_ptr<Expression> &arg: call->args) {
            args.push_back(simplify(arg));
        }
        return std::make_shared<Call>(call->signature, args);
    }

    return expression;
}

std::vector<std::shared_ptr<Expression>> Diff::getIndexesOfIndexedArg
        (std::shared_ptr<Expression> expression, std::string wrt) {
    std::vector<std::shared_ptr<Expression>> result;
    getIndexesOfIndexedArg(expression, wrt, result);
    return result;
}

void Diff::getIndexesOfIndexedArg
        (std::shared_ptr<Expression> expr, std::string wrt, std::vector<std::shared_ptr<Expression>> &found) {
    if (expr->getType() == Expression::BINARY_OPERATOR) {
        std::shared_ptr<BinaryOperator> op = std::dynamic_pointer_cast<BinaryOperator>(expr);

        if (op->op == BinaryOperator::INDEXING) {
            if (op->left->getType() == Expression::VARIABLE) {
                std::shared_ptr<Variable> opVar = std::dynamic_pointer_cast<Variable>(op->left);
                if (opVar->name == wrt) {
                    found.push_back(op->right);
                }
            }
            return;
        }

        getIndexesOfIndexedArg(op->left, wrt, found);
        getIndexesOfIndexedArg(op->right, wrt, found);
    } else if (expr->getType() == Expression::UNARY_OPERATOR) {
        std::shared_ptr<UnaryOperator> op = std::dynamic_pointer_cast<UnaryOperator>(expr);
        getIndexesOfIndexedArg(op->expr, wrt, found);
    } else if (expr->getType() == Expression::CALL) {
        std::shared_ptr<Call> call = std::dynamic_pointer_cast<Call>(expr);
        for (std::shared_ptr<Expression> &arg: call->args) {
            getIndexesOfIndexedArg(arg, wrt, found);
        }
    }
}


