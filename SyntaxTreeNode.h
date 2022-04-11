#ifndef DIFFERENTIATOR_SYNTAX_TREE_NODE_H
#define DIFFERENTIATOR_SYNTAX_TREE_NODE_H

#include <memory>
#include <string>
#include <exception>
#include <memory>
#include <vector>
#include <sstream>
#include "Context.h"


class ParsingException : public std::exception
{
public:
    std::string error;
    std::string message;

    explicit ParsingException(std::string error): error(error), message(error) {}

    ParsingException(std::string error, const std::string &file, int lineN, int charN): error(error) {
        std::shared_ptr<char[]> buf( new char[300] );
        int size = sprintf(buf.get(), "Error parsing file '%s' L%d C%d: %s",
                           file.c_str(), lineN, charN, error.c_str());
        message = std::string(buf.get(), buf.get() + size);
    }

    const char *what() const noexcept override
    {
        return message.c_str();
    }
};

struct SyntaxTreeNode {
public:
    virtual std::string to_string() = 0;

    virtual SyntaxTreeNode *copy() = 0;

    static void indent(std::string& value) {
        size_t pos = 0;
        while (pos != std::string::npos) {
            value.insert(pos, 1, '\t');
            pos = value.find('\n', pos);
            if (pos != std::string::npos) ++pos;
        }
    }
};

struct Expression: virtual SyntaxTreeNode {
    enum ExpressionType {
        UNARY_OPERATOR,
        BINARY_OPERATOR,
        ELEMENTARY_VALUE,
        VARIABLE,
        VARIABLE_DECLARATION,
        CALL,
        OTHER
    };

    virtual ExpressionType getType() = 0;

    Expression *copy() override = 0;

    static std::shared_ptr<Expression> add(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right);
    static std::shared_ptr<Expression> subtract(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right);
    static std::shared_ptr<Expression> multiply(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right);
    static std::shared_ptr<Expression> divide(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right);
};

struct Call: virtual Expression {
    FunctionSignature signature;
    std::vector<std::shared_ptr<Expression>> args;

    explicit Call(FunctionSignature& signature): signature(signature) {};
    Call(FunctionSignature &signature, std::vector<std::shared_ptr<Expression>> args):
            signature(signature), args(std::move(args)) {}
    Call(FunctionSignature &signature, std::shared_ptr<Expression> arg0): signature(signature) {
        args.push_back(std::move(arg0));
    }
    Call(FunctionSignature &signature, std::shared_ptr<Expression> arg0, std::shared_ptr<Expression> arg1):
            Call(signature, std::move(arg0)) {
        args.push_back(std::move(arg1));
    }

    ExpressionType getType() override {
        return CALL;
    }

    std::string to_string(bool methodCall, bool constructorCall) {
        std::ostringstream result;
        if (methodCall) {
            size_t i = signature.name.find_last_of(':');
            if (i == std::string::npos) { i = 0; }
            else { ++i; }
            result << signature.name.substr(i);
        } else if (!constructorCall) {
            result << signature.name;
        }

        result << '(';
        for (int i = 0; i < args.size(); ++i) {
            if (i != 0) result << ", ";
            result << args[i]->to_string();
        }
        result << ')';
        return result.str();
    }

    std::string to_string() override {
        return to_string(false, false);
    }

    Call *copy() override {
        std::vector<std::shared_ptr<Expression>> copyArgs;
        for (auto &arg: args) {
            copyArgs.push_back(std::shared_ptr<Expression>(arg->copy()));
        }
        return new Call(signature, copyArgs);
    }
};

struct Variable: virtual Expression {
    Type type;
    std::string name;
    bool declaration;
    std::shared_ptr<Call> constructorCall;

    Variable() = delete;
    Variable(Type type, std::string name, bool declaration=false):
            type(std::move(type)), name(std::move(name)), declaration(declaration) {};
    Variable(Type type, std::string name, bool declaration, std::shared_ptr<Call> constructorCall):
            type(std::move(type)), name(std::move(name)), declaration(declaration), constructorCall(std::move(constructorCall)) {}

    ExpressionType getType() override {
        return declaration ? VARIABLE_DECLARATION : VARIABLE;
    }

    std::string to_string() override {
        if (declaration) {
            if (constructorCall != nullptr) {
                return type.to_string() + " " + name + constructorCall->to_string(false, true);
            }
            return type.to_string() + " " + name;
        }
        return name;
    }

    Variable *copy() override {
        return new Variable(type, name);
    }
};

struct ElementaryValue: virtual Expression {
    ExpressionType getType() override {
        return Expression::ExpressionType::ELEMENTARY_VALUE;
    }
};

struct Number: virtual ElementaryValue {
    double value;

    explicit Number(double value): value(value) {};
    explicit Number(const std::string value): value(std::atof(value.c_str())){};

    std::string to_string() override {
        std::ostringstream output;
        output << value;
        return output.str();
    };

    Number* copy() override {
        return new Number(value);
    }

    bool isZero() {
        return value == 0;
    }

    bool isOne() {
        return value == 1;
    }
};

struct Operator: virtual Expression {
    virtual int getOperatorPrecedence() = 0;

    static int comparePrecedence(Operator *op1, Operator *op2) {
        return op1->getOperatorPrecedence() - op2->getOperatorPrecedence();
    }
};

struct UnaryOperator: virtual Operator {
    enum Operation {
        PLUS,
        MINUS,
        NOT,
        PLUS_PLUS,
        MINUS_MINUS,
        BRACES
    };

    static Operation parseOperation(std::string& opString) {
        if (opString == "+") {
            return PLUS;
        } else if (opString == "-") {
            return MINUS;
        } else if (opString == "!") {
            return NOT;
        } else if (opString == "++") {
            return PLUS_PLUS;
        } else if (opString == "--") {
            return MINUS_MINUS;
        }
        throw ParsingException("Supplied unsupported operator: '" + opString + "'");
    }

    static std::string operatorToString(Operation op) {
        if (op == PLUS) {
            return "+";
        } else if (op == MINUS) {
            return "-";
        } else if (op == NOT) {
            return "!";
        } else if (op == PLUS_PLUS) {
            return "++";
        } else if (op == MINUS_MINUS) {
            return "--";
        }
        return "";
    }

    Operation op;
    std::shared_ptr<Expression> expr;
    bool suffix = false;

    UnaryOperator() = delete;
    UnaryOperator(Operation op, std::shared_ptr<Expression> expr): op(op), expr(std::move(expr)) {}
    UnaryOperator(std::string opString, std::shared_ptr<Expression> expr):
            UnaryOperator(parseOperation(opString), std::move(expr)) {}
    UnaryOperator(Operation op, std::shared_ptr<Expression> expr, bool suffix): UnaryOperator(op, std::move(expr)) {
        if (suffix && op != PLUS_PLUS && op != MINUS_MINUS) {
            throw ParsingException("The operator '" + operatorToString(op) + "' does not have a suffix form");
        }
    }
    UnaryOperator(std::string opString, std::shared_ptr<Expression> expr, bool suffix):
            UnaryOperator(parseOperation(opString), std::move(expr), suffix) {}

    int getOperatorPrecedence() override {
        if (op == BRACES) {
            return 0;
        } else if (suffix && (op == PLUS_PLUS || op == MINUS_MINUS)) {
            return 2;
        } else if (op == PLUS_PLUS || op == MINUS_MINUS || op == PLUS || op == MINUS || op == NOT) {
            return 3;
        }
        return 100;
    }

    ExpressionType getType() override {
        return Expression::ExpressionType::UNARY_OPERATOR;
    }

    std::string to_string() override {
        if (op == BRACES) {
            return "(" + expr->to_string() + ")";
        }
        if (suffix) {
            return expr->to_string() + operatorToString(op);
        }
        return operatorToString(op) + expr->to_string();
    }

    UnaryOperator *copy() override {
        return new UnaryOperator(op, std::shared_ptr<Expression>(expr->copy()), suffix);
    }
};

struct BinaryOperator: virtual Operator {
    enum Operation {
        PLUS, MINUS,
        MULTIPLY, DIVIDE,
        IS_EQUAL, NOT_EQUALS,
        LESS, MORE, LESS_EQUALS, MORE_EQUALS,
        AND, OR,
        EQUALS, PLUS_EQUALS, MINUS_EQUALS, MULTIPLY_EQUALS, DIVIDE_EQUALS,
        INDEXING, POINT
    };

    static Operation parseOperation(std::string& opString) {
        if (opString == "+") {
            return PLUS;
        } else if (opString == "-") {
            return MINUS;
        } else if (opString == "*") {
            return MULTIPLY;
        } else if (opString == "/") {
            return DIVIDE;
        } else if (opString == "==") {
            return IS_EQUAL;
        } else if (opString == "!=") {
            return NOT_EQUALS;
        } else if (opString == "<") {
            return LESS;
        } else if (opString == ">") {
            return MORE;
        } else if (opString == "<=") {
            return LESS_EQUALS;
        } else if (opString == ">=") {
            return MORE_EQUALS;
        } else if (opString == "&&") {
            return AND;
        } else if (opString == "||") {
            return OR;
        } else if (opString == "=") {
            return EQUALS;
        } else if (opString == "+=") {
            return PLUS_EQUALS;
        } else if (opString == "-=") {
            return MINUS_EQUALS;
        } else if (opString == "*=") {
            return MULTIPLY_EQUALS;
        } else if (opString == "/=") {
            return DIVIDE_EQUALS;
        } else if (opString == ".") {
            return POINT;
        }
        throw ParsingException("Supplied unsupported operator: '" + opString + "'");
    }

    static std::string operatorToString(Operation op) {
        switch(op) {
            case PLUS: return "+";
            case MINUS: return "-";
            case MULTIPLY: return "*";
            case DIVIDE: return "/";
            case IS_EQUAL: return "==";
            case NOT_EQUALS: return "!=";
            case LESS: return "<";
            case MORE: return ">";
            case LESS_EQUALS: return "<=";
            case MORE_EQUALS: return ">=";
            case AND: return "&&";
            case OR: return "||";
            case EQUALS: return "=";
            case PLUS_EQUALS: return "+=";
            case MINUS_EQUALS: return "-=";
            case MULTIPLY_EQUALS: return "*=";
            case DIVIDE_EQUALS: return "/=";
            case POINT: return ".";
            default: return "";
        }
    }

    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    Operation op;

    BinaryOperator() = delete;
    BinaryOperator(Operation op, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right):
            op(op), left(std::move(left)), right(std::move(right)) {}
    BinaryOperator(std::string opString, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right):
            left(std::move(left)), right(std::move(right)) {
        op = parseOperation(opString);
    }

    int getOperatorPrecedence() override {
        if (op == INDEXING || op == POINT) {
            return 2;
        } if (op == MULTIPLY || op == DIVIDE) {
            return 5;
        } else if (op == PLUS || op == MINUS) {
            return 6;
        } else if (op == LESS || op == LESS_EQUALS || op == MORE || op == MORE_EQUALS) {
            return 9;
        } else if (op == IS_EQUAL || op == NOT_EQUALS) {
            return 10;
        } else if (op == AND) {
            return 14;
        } else if (op == OR) {
            return 15;
        } else if (op == EQUALS || op == PLUS_EQUALS || op == MINUS_EQUALS ||
                   op == MULTIPLY_EQUALS || op == DIVIDE_EQUALS) {
            return 16;
        }
        return 100;
    }

    ExpressionType getType() override {
        return Expression::ExpressionType::BINARY_OPERATOR;
    }

    std::string to_string() override {
        std::ostringstream result;
        if (op == INDEXING) {
            result << left->to_string() << '[' << right->to_string() << ']';
            return result.str();
        } else if (op == POINT) {
            result << left->to_string() << '.';
            if (right->getType() == CALL) {
                result << std::dynamic_pointer_cast<Call>(right)->to_string(true, false);
            } else {
                result << right->to_string();
            }
            return result.str();
        }

        auto *leftOp = dynamic_cast<BinaryOperator *>(left.get());
        if (leftOp != nullptr && comparePrecedence(this, leftOp) < 0) {
            result << '(' + leftOp->to_string() + ')';
        } else {
            result << left->to_string();
        }
        result << ' ' + operatorToString(op) + ' ';
        auto *rightOp = dynamic_cast<BinaryOperator *>(right.get());
        if (rightOp != nullptr && comparePrecedence(this, rightOp) < 0) {
            result << '(' + rightOp->to_string() + ')';
        } else {
            result << right->to_string();
        }
        return result.str();
    }

    BinaryOperator *copy() override {
        return new BinaryOperator(op, std::shared_ptr<Expression>(left->copy()), std::shared_ptr<Expression>(right->copy()));
    }
};

struct Statement: virtual SyntaxTreeNode {
    enum StatementType {
        EXPRESSION,
        BLOCK,
        IF,
        WHILE_LOOP,
        FOR_LOOP,
        BREAK,
        RETURN,
        COMMENT,
        INCLUDE,
        FUNCTION,
        FUNCTION_DECLARATION,
        OTHER,
    };

    virtual StatementType getType() = 0;

    virtual bool isFileStatement() {
        return false;
    };

    virtual bool isFunctionStatement() {
        return false;
    };

    Statement *copy() override = 0;
};

struct BreakStatement: virtual Statement {
    StatementType getType() override {
        return BREAK;
    }

    bool isFunctionStatement() override {
        return true;
    }

    std::string to_string() override {
        return "break;";
    }

    Statement *copy() override {
        return new BreakStatement();
    }
};

struct ExpressionStatement: virtual Statement {
    std::shared_ptr<Expression> expr;

    ExpressionStatement() = delete;
    explicit ExpressionStatement(std::shared_ptr<Expression> expr): expr(std::move(expr)) {}

    StatementType getType() override {
        return EXPRESSION;
    }

    bool isFunctionStatement() override {
        return true;
    }

    bool isFileStatement() override {
        return true;
    }

    std::string to_string() override {
        return expr->to_string() + ';';
    }

    ExpressionStatement *copy() override {
        return new ExpressionStatement(std::shared_ptr<Expression>(expr->copy()));
    }
};

struct ReturnStatement: virtual Statement {
    std::shared_ptr<Expression> expr;

    ReturnStatement() = delete;
    explicit ReturnStatement(std::shared_ptr<Expression> expr): expr(std::move(expr)){}

    StatementType getType() override {
        return RETURN;
    }

    bool isFunctionStatement() override {
        return true;
    }

    std::string to_string() override {
        return "return " + expr->to_string() + ';';
    }

    ReturnStatement *copy() override {
        return new ReturnStatement(std::shared_ptr<Expression>(expr->copy()));
    }
};

struct BlockStatement: virtual Statement {
    std::vector<std::shared_ptr<Statement>> statements;

    BlockStatement() = delete;
    explicit BlockStatement(std::vector<std::shared_ptr<Statement>> statements): statements(std::move(statements)) {}

    StatementType getType() override {
        return BLOCK;
    }

    bool isFunctionStatement() override {
        return true;
    }

    std::string to_string() override {
        std::ostringstream result;
        result << "{\n";
        for (auto &statement: statements) {
            std::string string = statement->to_string();
            indent(string);
            result << string << "\n";
        }
        result << "}";
        return result.str();
    }

    BlockStatement *copy() override {
        std::vector<std::shared_ptr<Statement>> copyStatements;
        for (auto &statement: statements) {
            copyStatements.push_back(std::shared_ptr<Statement>(statement->copy()));
        }
        return new BlockStatement(copyStatements);
    }
};

struct ConditionalStatement: virtual Statement {
    bool repeat = false;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Statement> statement;
    std::shared_ptr<Statement> elseStatement;

    ConditionalStatement() = delete;
    ConditionalStatement(bool repeat, std::shared_ptr<Expression> condition, std::shared_ptr<Statement> statement):
            repeat(repeat), condition(std::move(condition)), statement(std::move(statement)) {};
    ConditionalStatement(bool repeat, std::shared_ptr<Expression> condition,
                         std::shared_ptr<Statement> statement, std::shared_ptr<Statement> elseStatement):
            repeat(repeat), condition(std::move(condition)), statement(std::move(statement)), elseStatement(std::move(elseStatement)) {
        if (repeat && elseStatement != nullptr) {
            throw ParsingException("Else statement is not available for while loop");
        }
    };

    StatementType getType() override {
        return repeat ? WHILE_LOOP : IF;
    }

    bool isFunctionStatement() override {
        return true;
    }

    std::string to_string() override {
        std::ostringstream result;
        result << (repeat ? "while" : "if") << " (" << condition->to_string() << ") ";
        result << statement->to_string();
        if (elseStatement != nullptr) {
            result << " else " << elseStatement->to_string();
        }
        return result.str();
    }

    ConditionalStatement *copy() override {
        return new ConditionalStatement(repeat, std::shared_ptr<Expression>(condition->copy()),
            std::shared_ptr<Statement>(statement->copy()),
            elseStatement == nullptr ? nullptr : std::shared_ptr<Statement>(elseStatement->copy()));
    }
};

struct ForLoop: virtual Statement {
    std::shared_ptr<Statement> definition;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> expr;
    std::shared_ptr<Statement> statement;

    ForLoop() = delete;
    ForLoop(std::shared_ptr<Statement> definition, std::shared_ptr<Expression> condition,
                std::shared_ptr<Expression> expr, std::shared_ptr<Statement> statement):
            definition(std::move(definition)), condition(std::move(condition)), expr(std::move(expr)), statement(std::move(statement)) {};

    StatementType getType() override {
        return FOR_LOOP;
    }

    bool isFunctionStatement() override {
        return true;
    }

    std::string to_string() override {
        std::ostringstream result;
        result << "for (" << definition->to_string() << " ";
        result << condition->to_string() << "; " << expr->to_string() << ") ";
        result << statement->to_string();
        return result.str();
    }

    ForLoop *copy() override {
        return new ForLoop(std::shared_ptr<Statement>(definition->copy()),
                std::shared_ptr<Expression>(condition->copy()),
                std::shared_ptr<Expression>(expr->copy()),
                std::shared_ptr<Statement>(statement->copy()));
    }
};

struct Comment: virtual Statement {
    std::string commentText;
    bool multiLine = false;

    Comment() = delete;
    explicit Comment(std::string commentText, bool multiLine=false):
            commentText(std::move(commentText)), multiLine(multiLine) {};

    StatementType getType() override {
        return COMMENT;
    }

    bool isFunctionStatement() override {
        return true;
    }

    bool isFileStatement() override {
        return true;
    }

    std::string to_string() override {
        if (multiLine) {
            return "/*\n" + commentText + "\n*/";
        } else {
            return "// " + commentText;
        }
    }

    Comment *copy() override {
        return new Comment(commentText, multiLine);
    }
};

struct Include: virtual Statement {
    std::string name;
    bool arrowInclude;

    Include() = delete;
    explicit Include(std::string name, bool arrowInclude=false): name(std::move(name)), arrowInclude(arrowInclude) {};

    StatementType getType() override {
        return INCLUDE;
    }

    std::string to_string() override {
        return std::string("#include ") + (arrowInclude ? '<' : '\"') + name + (arrowInclude ? '>' : '\"');
    }

    Include *copy() override {
        return new Include(name, arrowInclude);
    }
};

struct FunctionDeclaration: virtual Statement {
    std::string name;
    Type returnType;
    std::vector<std::shared_ptr<Variable>> params;

    FunctionDeclaration() = delete;
    FunctionDeclaration(std::string name, Type returnType):
            name(std::move(name)), returnType(std::move(returnType)) {}
    FunctionDeclaration(std::string name, Type returnType, std::vector<std::shared_ptr<Variable>> params):
            name(std::move(name)), returnType(std::move(returnType)), params(std::move(params)) {}

    FunctionSignature getSignature() {
        std::vector<Type> paramTypes;
        for (auto &param: params) {
            paramTypes.push_back(param->type);
        }
        return {name, paramTypes};
    }

    StatementType getType() override {
        return FUNCTION_DECLARATION;
    }

    bool isFileStatement() override {
        return true;
    }

    std::string to_string(bool semicolon) {
        std::ostringstream result;
        result << returnType.to_string() << ' ' << name << '(';
        for (int i = 0; i < params.size(); ++i) {
            if (i != 0) result << ", ";
            result << params[i]->to_string();
        }
        result << ")";
        if (semicolon) result << ";";
        return result.str();
    }

    std::string to_string() override {
        return to_string(true);
    }

    FunctionDeclaration *copy() override {
        std::vector<std::shared_ptr<Variable>> paramsCopy;
        for (auto &param: params) {
            paramsCopy.push_back(std::shared_ptr<Variable>(param->copy()));
        }
        return new FunctionDeclaration(name, returnType, paramsCopy);
    }
};

struct Function: virtual Statement {
    std::shared_ptr<FunctionDeclaration> declaration;
    std::shared_ptr<BlockStatement> block;
    std::shared_ptr<Context> context;

    explicit Function(std::shared_ptr<Context> context): context(std::move(context)) {}
    Function(std::shared_ptr<Context> context, std::shared_ptr<FunctionDeclaration> declaration, std::shared_ptr<BlockStatement> block):
        context(std::move(context)), declaration(std::move(declaration)), block(std::move(block)) {};

    StatementType getType() override {
        return FUNCTION;
    }

    bool isFileStatement() override {
        return true;
    }

    std::string to_string() override {
        std::ostringstream result;
        result << declaration->to_string(false) << " ";
        result << block->to_string();
        return result.str();
    }

    Function *copy() override {
        std::shared_ptr<Context> contextCopy = std::make_shared<Context>(*context);
        return new Function(contextCopy,
             std::shared_ptr<FunctionDeclaration>(declaration->copy()),
             std::shared_ptr<BlockStatement>(block->copy()));
    }
};

struct FileNode: virtual SyntaxTreeNode {
    std::string name;
    std::vector<std::shared_ptr<Statement>> statements;
    std::shared_ptr<Context> context;

    FileNode() = delete;
    FileNode(std::shared_ptr<Context> context, std::string name): context(std::move(context)), name(std::move(name)) {}
    FileNode(std::shared_ptr<Context> context, std::string name, std::vector<std::shared_ptr<Statement>> statements):
            context(std::move(context)), name(std::move(name)), statements(std::move(statements)) {}

    std::string to_string() override {
        std::string result;
        for (int i = 0; i < statements.size(); ++i) {
            Statement::StatementType type = statements[i]->getType();
            if (i != 0 && (type == Statement::FUNCTION || type == Statement::FUNCTION_DECLARATION)) {
                result += "\n";
            }
            result += statements[i]->to_string();
            result += "\n";
        }
        return result;
    }

    FileNode *copy() override {
        std::vector<std::shared_ptr<Statement>> statementsCopy;
        for (auto &statement: statements) {
            statementsCopy.push_back(std::shared_ptr<Statement>(statement->copy()));
        }
        std::shared_ptr<Context> contextCopy = std::make_shared<Context>(*context);
        return new FileNode(contextCopy, name, statementsCopy);
    }
};

#endif // DIFFERENTIATOR_SYNTAX_TREE_NODE_H
