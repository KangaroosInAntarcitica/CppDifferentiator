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

bool FileReader::isIdentifierStart(char c) {
    return isLowChar(c) || isUpperChar(c) || c == UNDERSCORE;
}

bool FileReader::isIdentifierMid(char c) {
    return isLowChar(c) || isUpperChar(c) || c == isNumber(c) || c == UNDERSCORE;
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

std::shared_ptr<Number> FileReader::parseNumber() {
    int start = charN;
    bool wasPointPresent = false;
    while (isNumber(nextChar)) {
        step();
        if (nextChar == POINT) {
            wasPointPresent = true;
            step();
        } else if (nextChar == 'e') {
            wasPointPresent = true;
            step();
            if (nextChar == '-') {
                step();
            }
        }
    }
    skipWhitespace();
    return std::make_shared<Number>(line.substr(start, charN - start));
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

    int endN = charN;
    if (skipSpace) {
        skipWhitespace();
    }
    return line.substr(start, endN - start);
}


Type FileReader::parseType(std::shared_ptr<Context> context, std::string type) {
    if (type.empty()) {
        if (isNumber(nextChar)) {
            return Type(parseNumber()->to_string());
        }

        type = parseIdentifier(true);
        if (!context->isTypePresent(type)) {
            throw ParsingException("Type '" + type + "' is not supported");
        }
    }

    std::vector<Type> typeParameters;
    if (nextChar == '<') {
        verifyNextCharIs('<');
        bool first = true;
        while (nextChar != '>') {
            if (!first) {
                verifyNextCharIs(',');
            }
            first = false;
            typeParameters.push_back(parseType(context));
        }
        verifyNextCharIs('>');
    }

    skipWhitespace();
    return {type, typeParameters};
}

std::string FileReader::parseOperator() {
    int start = charN;

    while (!isIdentifierStart(nextChar) && !isNumber(nextChar) && !isWhitespace(nextChar)) {
        if (nextChar == OPEN_ROUND || nextChar == CLOSE_ROUND) {
            throw ParsingException(std::string("Unsupported character '") + nextChar + "' for operator");
        }
        step();
    }

    int endN = charN;
    skipWhitespace();
    return line.substr(start, endN - start);
}

std::shared_ptr<Expression> FileReader::parseExpression
        (std::shared_ptr<Context> context, bool isFirst, bool missingAllowed, std::shared_ptr<Expression> left) {
    if (left == nullptr) {
        if (nextChar == OPEN_ROUND) {
            verifyNextCharIs(OPEN_ROUND);
            left = std::make_shared<UnaryOperator>(UnaryOperator::BRACES, parseExpression(context));
            verifyNextCharIs(CLOSE_ROUND);
        } else if (isIdentifierStart(nextChar)) {
            std::string name = parseIdentifier(true);

            if (context->isVariablePresent(name)) {
                left = context->getVariable(name);
            } else if (isFirst && context->isTypePresent(name)) {
                Type type = parseType(context, name);

                std::string varName = parseIdentifier(true);
                std::shared_ptr<Call> call;
                if (nextChar == OPEN_ROUND) {
                    call = parseCall(context, type.name);
                }
                context->addVariable(varName, std::make_shared<Variable>(type, varName));
                left = std::make_shared<Variable>(type, varName, true, call);
            } else if (nextChar == OPEN_ROUND) {
                left = parseCall(context, name);
            } else {
                throw ParsingException("Identifier '" + name + "' was not defined yet to be used");
            }
        } else if (isNumber(nextChar)) {
            left = parseNumber();
        }
    }

    if (nextChar == CLOSE_ROUND || nextChar == SEMI_COLON || nextChar == COMMA || nextChar == CLOSE_SQUARE) {
        if (left == nullptr && !missingAllowed) {
            throw ParsingException("Missing expression");
        }
        return left;
    }

    std::string op = parseOperator();
    if (left == nullptr) {
        std::shared_ptr<Expression> right = parseExpression(context);
        return std::make_shared<UnaryOperator>(op, right);
    }

    if (op[0] == POINT) {
        if (left == nullptr || left->getType() != Expression::VARIABLE) {
            throw ParsingException("Calling methods is only supported on variables");
        }
        std::shared_ptr<Variable> var = std::dynamic_pointer_cast<Variable>(left);
        std::shared_ptr<Call> call = parseCall(context, "", var);
        std::shared_ptr<Expression> result = std::make_shared<BinaryOperator>(BinaryOperator::POINT, var, call);
        if (nextChar == CLOSE_ROUND || nextChar == SEMI_COLON || nextChar == COMMA || nextChar == CLOSE_SQUARE) {
            return result;
        }
        return parseExpression(context, false, false, result);
    } else if (op[0] == OPEN_SQUARE) {
        std::shared_ptr<Expression> right = parseExpression(context);
        verifyNextCharIs(CLOSE_SQUARE);
        std::shared_ptr<Expression> result = std::make_shared<BinaryOperator>(BinaryOperator::INDEXING, left, right);
        if (nextChar == CLOSE_ROUND || nextChar == SEMI_COLON || nextChar == COMMA || nextChar == CLOSE_SQUARE) {
            return result;
        }
        return parseExpression(context, false, false, result);
    }

    if (nextChar == OPEN_ROUND || isIdentifierStart(nextChar) || isNumber(nextChar)) {
        std::shared_ptr<Expression> right = parseExpression(context);
        std::shared_ptr<BinaryOperator> result = std::make_shared<BinaryOperator>(op, left, right);
        if (right->getType() == Expression::BINARY_OPERATOR) {
            auto *rightOperator = dynamic_cast<BinaryOperator*>(right.get());
            if (BinaryOperator::comparePrecedence(result.get(), rightOperator) < 0) {
                result->right = rightOperator->left;
                rightOperator->left = result;
                return right;
            }
        }
        return result;
    } else {
        std::shared_ptr<Expression> result = std::make_shared<UnaryOperator>(op, left, true);
        if (nextChar == CLOSE_ROUND || nextChar == SEMI_COLON || nextChar == COMMA || nextChar == CLOSE_SQUARE) {
            return result;
        }
        return parseExpression(context, false, false, result);
    }
}

std::shared_ptr<Call> FileReader::parseCall(std::shared_ptr<Context> context, std::string name, std::shared_ptr<Variable> var) {
    if (name.empty()) {
        name = parseIdentifier(true);
    }
    if (var != nullptr) {
        name = var->type.name + "::" + name;
    }

    verifyNextCharIs(OPEN_ROUND);
    std::vector<std::shared_ptr<Expression>> args;
    bool first = true;
    while (nextChar != CLOSE_ROUND) {
        if (!first) {
            verifyNextCharIs(COMMA);
        }
        first = false;
        args.push_back(parseExpression(context));
    }
    verifyNextCharIs(CLOSE_ROUND);

    std::vector<Type> types;
    for (auto &arg: args) {
        types.emplace_back();
    }
    FunctionSignature signature(name, types);
    std::shared_ptr<FunctionSignature> function = context->findFunction(signature);
    if (function == nullptr) {
        throw ParsingException(
                "Couldn't find function with signature matching '" + signature.to_string() + "'");
    }
    signature = *function;
    return std::make_shared<Call>(signature, args);
}

std::shared_ptr<Statement> FileReader::parseStatement(std::shared_ptr<Context> context, bool functionStatement) {
    if (nextChar == OPEN_CURLY) {
        return parseBlock(context);
    } else if (nextChar == '/') {
        step();
        if (nextChar == '/') {
            verifyNextCharIs('/');
            int start = charN;
            while (!end && nextChar != '\n') {
                step();
            }
            std::shared_ptr<Comment> comment = std::make_shared<Comment>(line.substr(start, charN - start));
            skipWhitespace();
            return comment;
        }
        throw ParsingException(std::string("Unexpected char '") + nextChar + "'");
    }

    if (isIdentifierStart(nextChar)) {
        std::string identifier = parseIdentifier(false, false);

        if (identifier == "if" || identifier == "while") {
            if (!functionStatement) {
                throw ParsingException("This statement type is not allowed here");
            }
            skipWhitespace();
            bool isWhile = identifier == "while";
            verifyNextCharIs(OPEN_ROUND);
            std::shared_ptr<Expression> condition = parseExpression(context);
            verifyNextCharIs(CLOSE_ROUND);
            std::shared_ptr<Statement> statement = parseStatement(context);

            std::shared_ptr<Statement> elseStatement;
            if (identifier == "if") {
                identifier = parseIdentifier(false, false);
                if (identifier == "else") {
                    skipWhitespace();
                    elseStatement = parseStatement(context);
                } else {
                    stepBack(identifier.size());
                }
            }
            return std::make_shared<ConditionalStatement>(isWhile, condition, statement, elseStatement);
        } else if (identifier == "return") {
            if (!functionStatement) {
                throw ParsingException("This statement type is not allowed here");
            }
            skipWhitespace();
            std::shared_ptr<ReturnStatement> returnStatement = std::make_shared<ReturnStatement>(
                    parseExpression(context));
            verifyNextCharIs(SEMI_COLON);
            return returnStatement;
        } else if (identifier == "for") {
            if (!functionStatement) {
                throw ParsingException("This statement type is not allowed here");
            }
            skipWhitespace();
            verifyNextCharIs(OPEN_ROUND);
            std::shared_ptr<Statement> definition = parseStatement(context);
            std::shared_ptr<Expression> condition = parseExpression(context, false, true);
            verifyNextCharIs(SEMI_COLON);
            std::shared_ptr<Expression> expression = parseExpression(context, false, true);
            verifyNextCharIs(CLOSE_ROUND);
            std::shared_ptr<Statement> statement = parseStatement(context);
            std::shared_ptr<ForLoop> forLoop = std::make_shared<ForLoop>(definition, condition, expression, statement);
            return forLoop;
        }
        stepBack(identifier.size());
    }

    std::shared_ptr<Expression> expression = parseExpression(context, true);
    verifyNextCharIs(SEMI_COLON);
    return std::make_shared<ExpressionStatement>(expression);
}

std::shared_ptr<BlockStatement> FileReader::parseBlock(std::shared_ptr<Context> context) {
    verifyNextCharIs(OPEN_CURLY);
    std::vector<std::shared_ptr<Statement>> statements;
    while (nextChar != CLOSE_CURLY) {
        statements.push_back(parseStatement(context));
    }
    verifyNextCharIs(CLOSE_CURLY);
    return std::make_shared<BlockStatement>(statements);
}

std::shared_ptr<Variable> FileReader::parseVariable(std::shared_ptr<Context> context, bool declarationRequired) {
    Type type;
    std::string name;
    if (!isIdentifierStart(nextChar)) {
        throw ParsingException(std::string("Unexpected character '") + nextChar + "' in variable");
    }

    name = parseIdentifier(true);
    skipWhitespace();

    if (context->isVariablePresent(name)) {
        if (declarationRequired) {
            throw ParsingException(std::string("Expected variable declaration, but type missing"));
        }
        return context->getVariable(name);
    } else if (context->isTypePresent(name)) {
        type = parseType(context, name);
        name = parseIdentifier();
        context->addVariable(name, std::make_shared<Variable>(type, name));
        return std::make_shared<Variable>(type, name, true);
    } else {
        throw ParsingException(std::string("The variable '") + name + "' was not defined in this context");
    }
}

std::shared_ptr<Statement> FileReader::parseFunction(std::shared_ptr<Context> globalContext) {
    skipWhitespace();
    std::shared_ptr<Context> funcContext = std::make_shared<Context>(globalContext);

    Type returnType = parseType(funcContext);
    skipWhitespace();
    std::string name = parseIdentifier();
    verifyNextCharIs(OPEN_ROUND);
    skipWhitespace();

    std::vector<std::shared_ptr<Variable>> params;
    bool firstArgument = true;
    while (nextChar != CLOSE_ROUND) {
        if (!firstArgument) {
            verifyNextCharIs(COMMA);
        }
        std::shared_ptr<Variable> param = parseVariable(funcContext, true);
        params.push_back(param);
        firstArgument = false;
    }
    verifyNextCharIs(CLOSE_ROUND);

    std::shared_ptr<FunctionDeclaration> decl = std::make_shared<FunctionDeclaration>(name, returnType, params);

    if (nextChar == OPEN_CURLY) {
        return std::make_shared<Function>(funcContext, decl, parseBlock(funcContext));
    } else {
        verifyNextCharIs(SEMI_COLON);
        return decl;
    }
}

std::shared_ptr<Statement> FileReader::parseFileStatement(std::shared_ptr<Context> context) {
    if (nextChar == '#') {
        step();
        std::string identifier = parseIdentifier();
        if (identifier == "include") {
            std::shared_ptr<Include> include;
            if (nextChar == '<' || nextChar == '\"') {
                char open = nextChar;
                char close = nextChar == '<' ? '>' : '\"';
                step();

                int start = charN;
                while (nextChar != close && nextChar != '\n') {
                    step();
                }
                std::string name = line.substr(start, charN - start);
                verifyNextCharIs(close);
                include = std::make_shared<Include>(name, open == '<');
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
        step();
        if (nextChar == '/') {
            verifyNextCharIs('/');
            int start = charN;
            while (!end && nextChar != '\n') {
                step();
            }
            std::shared_ptr<Comment> comment = std::make_shared<Comment>(line.substr(start, charN - start));
            skipWhitespace();
            return comment;
        }
        throw ParsingException(std::string("Unexpected char '") + nextChar + "'");
    } else {
        return parseFunction(std::move(context));
    }
}

std::shared_ptr<FileNode> FileReader::parseFile(std::shared_ptr<Context> globalContext) {
    std::shared_ptr<Context> context = std::make_shared<Context>(globalContext);

    std::vector<std::shared_ptr<Statement>> statements;
    try {
        skipWhitespace();
        while (!end) {
            statements.push_back(parseFileStatement(context));
        }
    } catch (ParsingException& e) {
        throw ParsingException(e.error, filePath, lineN + 1, charN + 1);
    }

    return std::make_shared<FileNode>(context, filePath, statements);
}
