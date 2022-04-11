#include "DefaultFunctionDiffStorage.h"

std::shared_ptr<Expression> DefaultFunctionDiffStorage::CosDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    FunctionSignature sinSignature = FunctionSignature("std::sin", Type());
    std::shared_ptr<Expression> result = std::make_shared<Call>(sinSignature, arg);
    result = std::make_shared<UnaryOperator>(UnaryOperator::MINUS, result);
    return Expression::multiply(result, diff.diff(arg, context, wrt));
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::SinDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    FunctionSignature cosSignature = FunctionSignature("std::cos", Type());
    std::shared_ptr<Expression> result = std::make_shared<Call>(cosSignature, arg);
    return Expression::multiply(result, diff.diff(arg, context, wrt));
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::PowDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> first = call->args[0];
    std::shared_ptr<Expression> second = call->args[1];

    std::shared_ptr<Expression> left = std::make_shared<Call>(call->signature, first,
         Expression::subtract(second, std::make_shared<Number>("1")));
    left = Expression::multiply(second, left);
    left = Expression::multiply(left, diff.diff(first, context, wrt));
    FunctionSignature logSignature = FunctionSignature("std::log", Type());
    std::shared_ptr<Call> logCall = std::make_shared<Call>(logSignature, first);
    std::shared_ptr<Expression> right = Expression::multiply(call, logCall);
    right =  Expression::multiply(right, diff.diff(second, context, wrt));
    return  Expression::add(left, right);
}

std::shared_ptr<Expression> DefaultFunctionDiffStorage::LogDiffCalculator::calculate
        (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> arg = call->args[0];
    return Expression::divide(diff.diff(arg, context, wrt), arg);
}

std::shared_ptr<Expression>
DefaultFunctionDiffStorage::VectorConstructorDiffCalculator::calculate(std::shared_ptr<Call> call, Diff &diff,
                                                                       std::shared_ptr<Diff::DiffContext> context,
                                                                       std::shared_ptr<Variable> wrt) {
    return std::make_shared<Call>(call->signature, call->args[0], diff.diff(call->args[1], context, wrt));
}

std::shared_ptr<Expression>
DefaultFunctionDiffStorage::ExpDiffCalculator::calculate(std::shared_ptr<Call> call, Diff &diff,
                                                         std::shared_ptr<Diff::DiffContext> context,
                                                         std::shared_ptr<Variable> wrt) {
    return Expression::multiply(call, diff.diff(call->args[0], context, wrt));
}

std::shared_ptr<Expression>
DefaultFunctionDiffStorage::AbsDiffCalculator::calculate(std::shared_ptr<Call> call, Diff &diff,
                                                         std::shared_ptr<Diff::DiffContext> context,
                                                         std::shared_ptr<Variable> wrt) {
    std::shared_ptr<Expression> first = call->args[0];
    std::shared_ptr<Expression> sign = Expression::subtract(
            std::make_shared<BinaryOperator>(BinaryOperator::MORE, first, std::make_shared<Number>(0)),
            std::make_shared<BinaryOperator>(BinaryOperator::LESS, first, std::make_shared<Number>(0)));
    return Expression::multiply(sign, diff.diff(call->args[0], context, wrt));
}
