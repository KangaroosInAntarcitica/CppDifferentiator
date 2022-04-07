#include "DefaultFunctionDiffStorage.h"

std::shared_ptr<Expression> DefaultFunctionDiffStorage::CosDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    FunctionSignature sinSignature = FunctionSignature("std::sin", Type());
    std::shared_ptr<Expression> result = std::make_shared<Call>(sinSignature, arg);
    result = std::make_shared<UnaryOperator>(UnaryOperator::MINUS, result);
    return std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, result, diff.diff(arg, context, wrt));
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::SinDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    FunctionSignature cosSignature = FunctionSignature("std::cos", Type());
    std::shared_ptr<Expression> result = std::make_shared<Call>(cosSignature, arg);
    return std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, result, diff.diff(arg, context, wrt));
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::PowDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> first = call->args[0];
    std::shared_ptr<Expression> second = call->args[1];

    std::shared_ptr<Expression> left = std::make_shared<Call>(call->signature, first,
         std::make_shared<BinaryOperator>(BinaryOperator::MINUS, second, std::make_shared<Number>("1")));
    left = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, second, left);
    left = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, left, diff.diff(first, context, wrt));
    FunctionSignature logSignature = FunctionSignature("std::log", Type());
    std::shared_ptr<Call> logCall = std::make_shared<Call>(logSignature, first);
    std::shared_ptr<Expression> right = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, call, logCall);
    right = std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, right, diff.diff(second, context, wrt));
    return std::make_shared<BinaryOperator>(BinaryOperator::PLUS, left, right);
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::LogDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    return std::make_shared<BinaryOperator>(BinaryOperator::DIVIDE, diff.diff(arg, context, wrt), arg);
}
