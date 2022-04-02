#include "CppParser.h"

FileReader::FileReader(std::string& filePath): filePath(filePath) {
    ifile.open(filePath);

    if (ifile.fail()) {
        throw ParsingException("Error opening file '" + filePath + "'");
    }

    end = !std::getline(ifile, line);
    step();
}

void FileReader::step() {
    if (end) {
        throw ParsingException("File ended before expected");
    }

    if (charN == line.size() && !end) {
        end = !std::getline(ifile, line);
        charN = -1;
        lineN++;
    }
    if (!end) {
        if (charN == line.size() - 1) {
            nextChar = '\n';
            ++charN;
        } else {
            nextChar = line.at(++charN);
        }
    }
}

void FileReader::stepBack(int nSteps) {
    if (charN < nSteps) {
        throw ParsingException("Unable to step back before the line start");
    }
    charN = charN - nSteps;
    nextChar = line.at(charN);
}

void FileReader::skipWhitespace() {
    while (isWhitespace(nextChar) && !end) {
        step();
    }
}

bool FileReader::isNumber(char c) {
    return '0' <= c && c <= '9';
}

bool FileReader::isLowChar(char c) {
    return 'a' <= c && c <= 'z';
}

bool FileReader::isUpperChar(char c) {
    return 'A' <= c && c <= 'Z';
}

bool FileReader::isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\000';
}

bool FileReader::isUnaryOperator(char c) {
    return c == '+' || c == '-';
}

bool FileReader::isOperator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/';
}

void FileReader::verifyNextCharIs(char c, bool skipSpace) {
    if (nextChar != c) {
        throw ParsingException(std::string("Expected '") + c + "', but '" + nextChar + "' was found");
    }
    step();
    if (skipSpace) {
        skipWhitespace();
    }
}

Expression* FileReader::parseNumber() {
    int start = charN;
    while (isNumber(nextChar) || nextChar == POINT) {
        step();
    }
    return new Number(line.substr(start, charN - start));
}

Expression* FileReader::parseExpression(Context& context) {
    Expression* left = nullptr;
    skipWhitespace();

    if (nextChar == OPEN_ROUND) {
        step();
        auto* leftOperator = new UnaryOperator();
        leftOperator->expr = parseExpression(context);
        leftOperator->op = UnaryOperator::Operation::BRACES;
        left = leftOperator;
        verifyNextCharIs(CLOSE_ROUND);
        skipWhitespace();
    } else if (isLowChar(nextChar) || isUpperChar(nextChar) || nextChar == UNDERSCORE) {
        std::string name = parseIdentifier(true);
        if (context.isVariablePresent(name)) {
            left = context.definedVariables[name];
        } else if (context.isFunctionPresent(name)) {
            left = parseFunctionCall(context, context.getFunction(name));
        } else {
            throw ParsingException("Identifier '" + name + "' was not defined yet to be used");
        }
        skipWhitespace();
    } else if (isNumber(nextChar)) {
        left = parseNumber();
        skipWhitespace();
    }

    if (nextChar == CLOSE_ROUND || nextChar == SEMI_COLON || nextChar == COMMA) {
        if (left == nullptr) {
            throw ParsingException("Missing expression");
        }
        return left;
    }

    std::string op = parseOperator();
    skipWhitespace();
    Expression* right = parseExpression(context);
    if (left == nullptr) {
        auto* expression = new UnaryOperator();
        expression->op = UnaryOperator::parseOperation(op);
        expression->expr = right;
        return expression;
    } else {
        auto* expression = new BinaryOperator();
        BinaryOperator::Operation operation = BinaryOperator::parseOperation(op);
        expression->op = operation;
        auto* rightExpression = dynamic_cast<BinaryOperator*>(right);

        if (rightExpression == nullptr ||
                BinaryOperator::comparePrecedence(expression, rightExpression) >= 0) {
            expression->left = left;
            expression->right = right;
            expression->op = operation;
            return expression;
        } else {
            expression->left = left;
            expression->right = rightExpression->left;
            expression->op = operation;
            rightExpression->left = expression;
            return rightExpression;
        }
    }
}

Expression* FileReader::parseFunctionCall(Context &context, FunctionDeclaration *decl) {
    auto *call = new FunctionCall();
    call->declaration = decl;

    verifyNextCharIs(OPEN_ROUND);
    while (nextChar != CLOSE_ROUND) {
        if (!call->args.empty()) {
            verifyNextCharIs(COMMA);
        }
        call->args.push_back(parseExpression(context));
    }
    verifyNextCharIs(CLOSE_ROUND);

    if (call->args.size() != decl->params.size()) {
        throw ParsingException("Function definition has " + std::to_string(decl->params.size()) +
                               " parameters, but call provided " + std::to_string(call->args.size()));
    }
    return call;
}

Statement *FileReader::parseStatement(Context& context) {
    if (nextChar == OPEN_CURLY) {
        return parseBlock(context);
    }

    std::string identifier = parseIdentifier(false, false);

    if (identifier == "if" || identifier == "while") {
        skipWhitespace();
        ConditionalStatement *statement = new ConditionalStatement;
        statement->repeat = identifier == "while";
        verifyNextCharIs(OPEN_ROUND);
        statement->condition = parseExpression(context);
        verifyNextCharIs(CLOSE_ROUND);
        statement->statement = parseStatement(context);

        if (identifier == "if") {
            identifier = parseIdentifier(false, false);
            if (identifier == "else") {
                skipWhitespace();
                statement->elseStatement = parseStatement(context);
            } else {
                stepBack(identifier.size());
            }
        }
        return statement;
    } else if (identifier == "return") {
        Expression *expression = parseExpression(context);
        ReturnStatement *statement = new ReturnStatement(expression);
        verifyNextCharIs(SEMI_COLON);
        return statement;
    } else if (identifier == "for") {
        skipWhitespace();
        verifyNextCharIs(OPEN_ROUND);
        Statement *definition = parseStatement(context);
        Expression *condition = parseExpression(context);
        verifyNextCharIs(SEMI_COLON);
        Expression *expression = parseExpression(context);
        ForLoop *forLoop = new ForLoop(definition, condition, expression);
        forLoop->statement = parseStatement(context);
        return forLoop;
    } else {
        stepBack(identifier.size());
        AssignmentStatement* statement = new AssignmentStatement;
        statement->var = parseVariable(context, false);

        Variable* variable = dynamic_cast<Variable*>(statement->var);

        if (nextChar == SEMI_COLON) {
            return statement;
        }
        std::string op = parseOperator();
        AssignmentStatement::AssignmentOperator operation = AssignmentStatement::parseOperation(op);
        skipWhitespace();
        Expression *expr = parseExpression(context);
        skipWhitespace();

        verifyNextCharIs(SEMI_COLON);
        statement->op = operation;
        statement->expr = expr;
        return statement;
    }
}

BlockStatement *FileReader::parseBlock(Context &context) {
    verifyNextCharIs(OPEN_CURLY);
    BlockStatement *block = new BlockStatement();
    while (nextChar != CLOSE_CURLY) {
        Statement* statement = parseStatement(context);
        block->statements.push_back(statement);
    }
    verifyNextCharIs(CLOSE_CURLY);
    return block;
}

Expression* FileReader::parseVariable(Context& context, bool declarationRequired) {
    std::string type;
    std::string name;
    if (!isLowChar(nextChar) && !isUpperChar(nextChar) && nextChar != UNDERSCORE) {
        throw ParsingException(std::string("Unexpected character '") + nextChar + "' in variable");
    }

    name = parseIdentifier(true);
    skipWhitespace();

    if (isLowChar(nextChar) || isUpperChar(nextChar) || nextChar == UNDERSCORE) {
        type = name;
        name = parseIdentifier();
        skipWhitespace();
    }

    if (!type.empty()) {
        Type variableType = Type(type);
        auto *variable = new Variable(variableType, name);
        context.definedVariables[name] = variable;
        return new VariableDeclaration(variable);
    } else if (context.isVariablePresent(name)) {
        if (declarationRequired) {
            throw ParsingException(std::string("Expected variable declaration, but type missing"));
        }
        return context.definedVariables[name];
    } else {
        throw ParsingException(std::string("The variable '") + name + "' was not defined in this context");
    }
}

FileStatement* FileReader::parseFunction(Context& globalContext) {
    skipWhitespace();
    FunctionDeclaration* decl = new FunctionDeclaration;
    Context funcContext = globalContext;

    decl->returnType = parseType(funcContext);
    skipWhitespace();
    decl->name = parseIdentifier();
    verifyNextCharIs(OPEN_ROUND);
    skipWhitespace();

    bool firstArgument = true;
    while (nextChar != CLOSE_ROUND) {
        if (!firstArgument) {
            verifyNextCharIs(COMMA);
            skipWhitespace();
        }
        Expression* expression = parseVariable(funcContext, true);
        VariableDeclaration* declaration = dynamic_cast<VariableDeclaration*>(expression);
        decl->params.push_back(declaration->var);
        delete declaration;
        firstArgument = false;
    }
    verifyNextCharIs(CLOSE_ROUND);
    skipWhitespace();

    if (nextChar == OPEN_CURLY) {
        Function* function = new Function(decl);
        function->block = parseBlock(funcContext);
        function->context = funcContext;
        return function;
    } else {
        verifyNextCharIs(SEMI_COLON);

        return decl;
    }
}

Type FileReader::parseType(Context& context) {
    std::string type = parseIdentifier();
    if (!context.isTypePresent(type)) {
        throw ParsingException("Type '" + type + "' is not supported");
    }
    return Type(type);
}

std::string FileReader::parseOperator() {
    int start = charN;

    while (!isWhitespace(nextChar) && !isLowChar(nextChar) && !isUpperChar(nextChar) && nextChar != UNDERSCORE) {
        if (nextChar == OPEN_ROUND || nextChar == CLOSE_ROUND) {
            throw ParsingException(std::string("Unsupported character '") + nextChar + "' for operator");
        }
        step();
    }

    return line.substr(start, charN - start);
}

std::string FileReader::parseIdentifier(bool allowColon, bool skipSpace) {
    int start = charN;

    if (!isLowChar(nextChar) && !isUpperChar(nextChar) && nextChar != UNDERSCORE) {
        throw ParsingException(std::string("Invalid character '") + nextChar + "' in identifier");
    }
    step();

    while (true) {
        if (!isLowChar(nextChar) && !isUpperChar(nextChar) &&
            !isNumber(nextChar) && nextChar != UNDERSCORE && !(allowColon && nextChar == COLON)) {
            break;
        }
        step();
    }
    int end = charN;
    if (skipSpace) {
        skipWhitespace();
    }

    return line.substr(start, end - start);
}

FileStatement *FileReader::parseFileStatement(Context &globalContext) {
    if (nextChar == '#') {
        step();
        std::string identifier = parseIdentifier();
        if (identifier == "include") {
            Include *include;
            if (nextChar == '<') {
                step();
                std::string name = parseIdentifier();
                include = new Include(name, true);
                if (nextChar != '>') {
                    throw ParsingException(std::string("Include should be closed with '>', but got '") + nextChar + "'");
                }
                step();
            } else if (nextChar == '\"') {
                step();
                std::string name = parseIdentifier();
                include = new Include(name, false);
                if (nextChar != '\"') {
                    throw ParsingException(std::string("Include should be closed with '\"', but got '") + nextChar + "'");
                }
                step();
            } else {
                throw ParsingException(std::string("Include should be enclosed in quotation marks, but got '") + nextChar + "'");
            }
            if (nextChar == SEMI_COLON) {
                step();
                skipWhitespace();
            }
            return include;
        } else {
            throw ParsingException("File statement of type '#" + identifier + "' is not supported");
        }
    } else if (nextChar == '/') {
        verifyNextCharIs('/');
        verifyNextCharIs('/');
        while (nextChar != '\n' && !end) {
            step();
        }
        skipWhitespace();
        return nullptr;
    } else {
        return parseFunction(globalContext);
    }
}

FileNode FileReader::parseFile() {
    FileNode fileNode;

    Type *integer = new Type("int");
    Type *floating = new Type("float");
    Type *doubleFloating = new Type("double");
    fileNode.context.addType(integer);
    fileNode.context.addType(floating);
    fileNode.context.addType(doubleFloating);

    fileNode.context.addFunction(new FunctionDeclaration("std::cos", 1));
    fileNode.context.addFunction(new FunctionDeclaration("std::sin", 1));
    fileNode.context.addFunction(new FunctionDeclaration("std::pow", 2));
    fileNode.context.addFunction(new FunctionDeclaration("std::log", 1));

    fileNode.name = filePath;

    try {
        skipWhitespace();
        while (!end) {
            FileStatement *statement = parseFileStatement(fileNode.context);
            if (statement != nullptr) {
                fileNode.statements.push_back(statement);
            }
            skipWhitespace();
        }
    } catch (ParsingException e) {
        throw ParsingException(e.error, filePath, lineN + 1, charN + 1);
    }
    return fileNode;
}
