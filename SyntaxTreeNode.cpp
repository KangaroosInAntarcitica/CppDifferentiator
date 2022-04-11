#include "SyntaxTreeNode.h"

std::shared_ptr<Expression> Expression::add(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right) {
    return std::make_shared<BinaryOperator>(BinaryOperator::PLUS, std::move(left), std::move(right));
}

std::shared_ptr<Expression> Expression::subtract(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right) {
    return std::make_shared<BinaryOperator>(BinaryOperator::MINUS, std::move(left), std::move(right));
}

std::shared_ptr<Expression> Expression::multiply(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right) {
    return std::make_shared<BinaryOperator>(BinaryOperator::MULTIPLY, std::move(left), std::move(right));
}

std::shared_ptr<Expression> Expression::divide(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right) {
    return std::make_shared<BinaryOperator>(BinaryOperator::DIVIDE, std::move(left), std::move(right));
}
