#include "DefaultFunctionDiffStorage.h"

FunctionDiffStorage *DefaultFunctionDiffStorage::instance = new DefaultFunctionDiffStorage();

Expression *DefaultFunctionDiffStorage::CosDiffCalculator::calculate
        (FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) {
    Expression *arg = call->args[0];
    Expression *result = new FunctionCall(new FunctionDeclaration("std::sin", 1), arg);
    result = new UnaryOperator(UnaryOperator::MINUS, result);
    return new BinaryOperator(BinaryOperator::MULTIPLY, result, diff.diff(arg, context, wrt));
}

Expression *DefaultFunctionDiffStorage::SinDiffCalculator::calculate
        (FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) {
    Expression *arg = call->args[0];
    Expression *result = new FunctionCall(new FunctionDeclaration("std::cos", 1), arg);
    return new BinaryOperator(BinaryOperator::MULTIPLY, result, diff.diff(arg, context, wrt));
}

Expression *DefaultFunctionDiffStorage::PowDiffCalculator::calculate
        (FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) {
    Expression *first = call->args[0];
    Expression *second = call->args[1];

    Expression *left = new FunctionCall(call->declaration, first,
                                        new BinaryOperator(BinaryOperator::MINUS, second, new Number("1")));
    left = new BinaryOperator(BinaryOperator::MULTIPLY, second, left);
    left = new BinaryOperator(BinaryOperator::MULTIPLY, left, diff.diff(first, context, wrt));
    auto *log = new FunctionCall(new FunctionDeclaration("std::log", 1), first);
    Expression *right = new BinaryOperator(BinaryOperator::MULTIPLY, call, log);
    right = new BinaryOperator(BinaryOperator::MULTIPLY, right, diff.diff(second, context, wrt));
    return new BinaryOperator(BinaryOperator::PLUS, left, right);
}

Expression *DefaultFunctionDiffStorage::LogDiffCalculator::calculate
        (FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) {
    Expression *arg = call->args[0];
    return new BinaryOperator(BinaryOperator::DIVIDE, diff.diff(arg, context, wrt), arg);
}
