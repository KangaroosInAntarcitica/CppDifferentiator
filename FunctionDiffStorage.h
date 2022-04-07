#ifndef FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H
#define FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H

#include <unordered_map>
#include <string>
#include <vector>
#include "SyntaxTreeNode.h"
#include "Diff.h"


class FunctionDiffStorage {
public:
    struct DiffCalculator {
        virtual std::shared_ptr<Expression> calculate
            (std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) = 0;
    };

protected:
    std::shared_ptr<Context> context;
    std::unordered_map<FunctionSignature, std::shared_ptr<DiffCalculator>> functionDiffCalculators;

public:
    FunctionDiffStorage(std::shared_ptr<Context> context): context(context) {}

    void addDiffCalculator(FunctionSignature signature, DiffCalculator *diffCalculator) {
        functionDiffCalculators[signature] = std::shared_ptr<DiffCalculator>(diffCalculator);
    }

    virtual std::shared_ptr<Expression> convert(std::shared_ptr<Call> call, Diff &diff,
                                        std::shared_ptr<Diff::DiffContext> diffContext, std::shared_ptr<Variable> wrt) {
        std::shared_ptr<FunctionSignature> signature = context->findFunction(call->signature);
        if (signature == nullptr) return nullptr;
        return functionDiffCalculators[*signature]->calculate(call, diff, diffContext, wrt);
    }
};

#endif //FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H
