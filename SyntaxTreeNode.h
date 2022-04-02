#ifndef DIFFERENTIATOR_SYNTAX_TREE_NODE_H
#define DIFFERENTIATOR_SYNTAX_TREE_NODE_H

#include <string>
#include <exception>
#include <memory>
#include <vector>
#include <sstream>

class ParsingException : public std::exception
{
public:
    std::string error;
    std::string message;

    ParsingException(std::string error): error(error), message(error) {}

    ParsingException(std::string error, std::string file, int lineN, int charN): error(error) {
        std::unique_ptr<char[]> buf( new char[300] );
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

    static void indent(std::string& value) {
        size_t pos = 0;
        while (pos != std::string::npos) {
            value.insert(pos, 1, '\t');
            pos = value.find('\n', pos);
            if (pos != std::string::npos) ++pos;
        }
    }
};

struct ParentTreeNode: virtual SyntaxTreeNode {};

struct Expression: virtual SyntaxTreeNode {
    enum ExpressionType {
        UNARY_OPERATOR,
        BINARY_OPERATOR,
        ELEMENTARY_VALUE,
        VARIABLE,
        VARIABLE_DECLARATION,
        FUNCTION_CALL,
        OTHER
    };

    virtual ExpressionType getType() = 0;
};

struct Type: virtual SyntaxTreeNode {
    std::string name;
    std::vector<std::string> generics;

    Type() = default;
    explicit Type(std::string name): name(name) {}

    virtual std::string to_string() {
        std::string result = name;
        if (!generics.empty()) {
            result += '<';
            for (int i = 0; i < generics.size(); ++i) {
                if (i != 0) result += ", ";
                result += generics[i];
            }
            result += '>';
        }
        return result;
    };
};

struct Variable: virtual Expression {
    Type type;
    std::string name;

    Variable() = default;
    Variable(Type type, std::string name): type(type), name(name) {};

    virtual ExpressionType getType() override {
        return Expression::ExpressionType::VARIABLE;
    }

    virtual std::string to_string() override {
        return name;
    }
};

struct FileStatement: virtual SyntaxTreeNode {
    enum FileStatementType {
        FUNCTION_DECLARATION,
        FUNCTION,
        INCLUDE,
        OTHER
    };

    virtual FileStatementType getType() = 0;
};

struct FunctionDeclaration: virtual FileStatement {
    std::string name;
    Type returnType;
    std::vector<Variable*> params;

    FunctionDeclaration() = default;
    FunctionDeclaration(std::string name): name(name){};
    FunctionDeclaration(std::string name, Type returnType): name(name), returnType(returnType) {}
    FunctionDeclaration(std::string name, int nParams): name(name) {
        for (int i = 0; i < nParams; ++i) {
            params.push_back(new Variable(Type(""), "param_" + std::to_string(i)));
        }
    }

    virtual FileStatementType getType() override {
        return FUNCTION_DECLARATION;
    }

    std::string to_string(bool semicolon) {
        std::string result;
        result += returnType.to_string() + ' ' + name + '(';
        for (int i = 0; i < params.size(); ++i) {
            if (i != 0) result += ", ";
            result += params[i]->type.to_string() + ' ' + params[i]->to_string();
        }
        result += ")";
        if (semicolon) result += ";";
        return result;
    }

    virtual std::string to_string() override {
        return to_string(true);
    }
};

struct Context {
public:
    std::unordered_map<std::string, Variable*> definedVariables;
    std::unordered_map<std::string, Type*> definedTypes;
    std::unordered_map<std::string, FunctionDeclaration*> definedFunctions;
    Context* parent = nullptr;

    void addVariable(Variable *var) {
        definedVariables[var->name] = var;
    }

    void addType(Type *type) {
        definedTypes[type->name] = type;
    }

    void addFunction(FunctionDeclaration *function) {
        definedFunctions[function->name] = function;
    }

    bool isVariablePresent(std::string& name) {
        return definedVariables.count(name) ||
            (parent != nullptr && parent->isVariablePresent(name));
    }

    bool isTypePresent(std::string& name) {
        return definedTypes.count(name) ||
            (parent != nullptr && parent->isTypePresent(name));
    }

    bool isFunctionPresent(std::string& name) {
        return definedFunctions.count(name) ||
            (parent != nullptr && parent->isFunctionPresent(name));
    }

    Variable* getVariable(std::string& name) {
        return definedVariables.count(name) > 0 ? definedVariables[name] :
            (parent == nullptr ? nullptr : parent->getVariable(name));
    }

    Type* getType(std::string& name) {
        return definedTypes.count(name) > 0 ? definedTypes[name] :
            (parent == nullptr ? nullptr : parent->getType(name));
    }

    FunctionDeclaration* getFunction(std::string &name) {
        return definedFunctions.count(name) > 0 ? definedFunctions[name] :
            (parent == nullptr ? nullptr : parent->getFunction(name));
    }
};

struct FunctionCall: virtual Expression {
    FunctionDeclaration *declaration;
    std::vector<Expression *> args;

    FunctionCall() = default;
    FunctionCall(FunctionDeclaration *declaration): declaration(declaration) {};
    FunctionCall(FunctionDeclaration *declaration, Expression *arg): declaration(declaration) {
        args.push_back(arg);
    }
    FunctionCall(FunctionDeclaration *declaration, Expression *arg1, Expression *arg2): FunctionCall(declaration, arg1) {
        args.push_back(arg2);
    }

    virtual ExpressionType getType() override {
        return ExpressionType::FUNCTION_CALL;
    }

    virtual std::string to_string() {
        std::string result = declaration->name + '(';
        for (int i = 0; i < args.size(); ++i) {
            if (i != 0) result += ", ";
            result += args[i]->to_string();
        }
        result += ')';
        return result;
    }
};

struct VariableDeclaration: virtual Expression {
    Variable* var;

    VariableDeclaration() = default;
    explicit VariableDeclaration(Variable *var): var(var) {};

    virtual ExpressionType getType() override {
        return Expression::ExpressionType::VARIABLE_DECLARATION;
    }

    virtual std::string to_string() {
        return var->type.to_string() + " " + var->to_string();
    }
};

struct ElementaryValue: virtual Expression {
    virtual ExpressionType getType() override {
        return Expression::ExpressionType::ELEMENTARY_VALUE;
    }
};

struct Number: virtual ElementaryValue {
    double value;

    Number() = default;
    explicit Number(std::string value): value(std::atof(value.c_str())){};

    virtual std::string to_string() {
        std::ostringstream output;
        output << value;
        return output.str();
    };
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
    Expression *expr;

    UnaryOperator() = default;
    UnaryOperator(Operation op, Expression *expr): op(op), expr(expr) {}
    UnaryOperator(std::string opString, Expression *expr): expr(expr) {
        op = parseOperation(opString);
    }

    virtual int getOperatorPrecedence() override {
        if (op == BRACES) {
            return 0;
        } else if (op == PLUS_PLUS || op == MINUS_MINUS) {
            return 2;
        } else if (op == PLUS || op == MINUS || op == NOT) {
            return 3;
        }
        return 100;
    }

    virtual ExpressionType getType() override {
        return Expression::ExpressionType::UNARY_OPERATOR;
    }

    virtual std::string to_string() override {
        if (op == BRACES) {
            return "(" + expr->to_string() + ")";
        }
        return operatorToString(op) + expr->to_string();
    }
};

struct BinaryOperator: virtual Operator {
    enum Operation {
        PLUS, MINUS,
        MULTIPLY, DIVIDE,
        EQUALS, NOT_EQUALS,
        LESS, MORE, LESS_EQUALS, MORE_EQUALS,
        AND, OR
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
            return EQUALS;
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
        }
        throw ParsingException("Supplied unsupported operator: '" + opString + "'");
    }

    static std::string operatorToString(Operation op) {
        if (op == PLUS) {
            return "+";
        } else if (op == MINUS) {
            return "-";
        } else if (op == MULTIPLY) {
            return "*";
        } else if (op == DIVIDE) {
            return "/";
        } else if (op == EQUALS) {
            return "==";
        } else if (op == NOT_EQUALS) {
            return "!=";
        } else if (op == LESS) {
            return "<";
        } else if (op == MORE) {
            return ">";
        } else if (op == LESS_EQUALS) {
            return "<=";
        } else if (op == MORE_EQUALS) {
            return ">=";
        } else if (op == AND) {
            return "&&";
        } else if (op == OR) {
            return "||";
        }
        return "";
    }

    Expression *left;
    Expression *right;
    Operation op;

    BinaryOperator() = default;
    BinaryOperator(Operation op, Expression *left, Expression *right): op(op), left(left), right(right) {}
    BinaryOperator(std::string opString, Expression *left, Expression *right): left(left), right(right) {
        op = parseOperation(opString);
    }

    int getOperatorPrecedence() override {
        if (op == MULTIPLY || op == DIVIDE) {
            return 5;
        } else if (op == PLUS || op == MINUS) {
            return 6;
        } else if (op == LESS || op == LESS_EQUALS || op == MORE || op == MORE_EQUALS) {
            return 9;
        } else if (op == EQUALS || op == NOT_EQUALS) {
            return 10;
        } else if (op == AND) {
            return 14;
        } else if (op == OR) {
            return 15;
        }
        return 100;
    }

    ExpressionType getType() override {
        return Expression::ExpressionType::BINARY_OPERATOR;
    }

    std::string to_string() override {
        std::string result;
        BinaryOperator *leftOp = dynamic_cast<BinaryOperator *>(left);
        if (leftOp != nullptr && comparePrecedence(this, leftOp) < 0) {
            result += '(' + leftOp->to_string() + ')';
        } else {
            result += left->to_string();
        }
        result += ' ' + operatorToString(op) + ' ';
        BinaryOperator *rightOp = dynamic_cast<BinaryOperator *>(right);
        if (rightOp != nullptr && comparePrecedence(this, rightOp) < 0) {
            result += '(' + rightOp->to_string() + ')';
        } else {
            result += right->to_string();
        }
        return result;
    }
};

struct Statement: virtual SyntaxTreeNode {
    enum StatementType {
        ASSIGNMENT,
        BLOCK,
        IF,
        WHILE_LOOP,
        FOR_LOOP,
        BREAK,
        RETURN,
        OTHER,
    };

    virtual StatementType getType() = 0;
};

struct AssignmentStatement: virtual Statement {
    enum AssignmentOperator {
        EQUALS,
        PLUS_EQUALS,
        MINUS_EQUALS,
        MULTIPLY_EQUALS,
        DIVIDE_EQUALS,
        OTHER
    };

    static AssignmentOperator parseOperation(std::string op) {
        if (op == "=") {
            return EQUALS;
        } else if (op == "+=") {
            return PLUS_EQUALS;
        } else if (op == "-=") {
            return MINUS_EQUALS;
        } else if (op == "*=") {
            return MULTIPLY_EQUALS;
        } else if (op == "/=") {
            return DIVIDE_EQUALS;
        } else {
            throw ParsingException("Supplied unsupported assignment operator: '" + op + "'");
        }
    }

    static std::string operatorToString(AssignmentOperator op) {
        if (op == EQUALS) {
            return "=";
        } else if (op == PLUS_EQUALS) {
            return "+=";
        } else if (op == MINUS_EQUALS) {
            return "-=";
        } else if (op == MULTIPLY_EQUALS) {
            return "*=";
        } else if (op == DIVIDE_EQUALS) {
            return "/=";
        }
        return "";
    }

    Expression* var = nullptr;
    AssignmentOperator op = EQUALS;
    Expression* expr = nullptr;

    virtual StatementType getType() override {
        return ASSIGNMENT;
    }

    virtual std::string to_string() {
        std::string result;
        if (var != nullptr) {
            result += var->to_string();
        }
        std::string opString = operatorToString(op);
        if (!opString.empty()) {
            if (!result.empty()) result += ' ';
            result += opString;
        }
        if (expr != nullptr) {
            if (!result.empty()) result += ' ';
            result += expr->to_string();
        }
        result += ';';
        return result;
    }
};

struct ReturnStatement: virtual Statement {
    Expression *expr = nullptr;

    ReturnStatement() = default;
    explicit ReturnStatement(Expression *expr): expr(expr){}

    StatementType getType() override {
        return RETURN;
    }

    std::string to_string() override {
        return "return " + expr->to_string() + ';';
    }
};

struct BlockStatement: virtual Statement {
    std::vector<Statement *> statements;

    BlockStatement() = default;

    StatementType getType() override {
        return BLOCK;
    }

    std::string to_string() override {
        std::ostringstream result;
        result << "{\n";
        for (Statement *statement: statements) {
            std::string string = statement->to_string();
            indent(string);
            result << string << "\n";
        }
        result << "}";
        return result.str();
    }
};

struct ConditionalStatement: virtual Statement {
    bool repeat = false;
    Expression *condition;
    Statement *statement;
    Statement *elseStatement = nullptr;

    ConditionalStatement() = default;
    ConditionalStatement(bool repeat): repeat(repeat) {};
    ConditionalStatement(bool repeat, Expression *condition): repeat(repeat), condition(condition){};

    virtual StatementType getType() override {
        return repeat ? WHILE_LOOP : IF;
    }

    virtual std::string to_string() override {
        std::ostringstream result;
        result << (repeat ? "while" : "if") << " (" << condition->to_string() << ") ";
        result << statement->to_string();
        if (elseStatement != nullptr) {
            result << " else " << elseStatement->to_string();
        }
        return result.str();
    }
};

struct ForLoop: virtual Statement {
    Statement *definition;
    Expression *condition;
    Expression *expr;
    Statement *statement;

    ForLoop() = default;
    ForLoop(Statement *definition, Expression *condition, Expression *expr):
        definition(definition), condition(condition), expr(expr) {};

    virtual StatementType getType() override {
        return FOR_LOOP;
    }

    virtual std::string to_string() override {
        std::ostringstream result;
        result << "for (" << definition->to_string();
        result << condition->to_string() << "; " << expr->to_string() << ") ";
        result << statement->to_string();
        return result.str();
    }
};

struct Function: virtual FileStatement {
    FunctionDeclaration *declaration;
    BlockStatement *block;
    Context context;

    Function() = default;
    Function(FunctionDeclaration *declaration): declaration(declaration) {};

    virtual FileStatementType getType() override {
        return FUNCTION;
    }

    virtual std::string to_string() {
        std::ostringstream result;
        result << declaration->to_string(false) << " ";
        result << block->to_string();
        return result.str();
    }
};

struct Include: virtual FileStatement {
    std::string name;
    bool useArrows;

    Include() = default;
    Include(std::string name): name(name), useArrows(false){};
    Include(std::string name, bool useArrows): name(name), useArrows(useArrows){};

    virtual FileStatementType getType() override {
        return INCLUDE;
    }

    virtual std::string to_string() override {
        return std::string("#include ") + (useArrows ? '<' : '\"') + name + (useArrows ? '>' : '\"');
    }
};

struct FileNode: virtual SyntaxTreeNode {
    std::string name;
    std::vector<FileStatement *> statements;
    Context context;

    virtual std::string to_string() {
        std::string result;
        for (int i = 0; i < statements.size(); ++i) {
            if (i != 0) result += "\n";
            result += statements[i]->to_string();
            result += "\n";
        }
        return result;
    }
};

#endif // DIFFERENTIATOR_SYNTAX_TREE_NODE_H
