#ifndef FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H
#define FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H

#include "FunctionDiffStorage.h"

class DefaultFunctionDiffStorage: public FunctionDiffStorage {
    struct CosDiffCalculator: virtual DiffCalculator {
        std::shared_ptr<Expression> calculate(std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) override;
    };

    struct SinDiffCalculator: virtual DiffCalculator {
        std::shared_ptr<Expression> calculate(std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) override;
    };

    struct PowDiffCalculator: virtual DiffCalculator {
        std::shared_ptr<Expression> calculate(std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context, std::shared_ptr<Variable> wrt) override;
    };

    struct LogDiffCalculator: virtual DiffCalculator {
        std::shared_ptr<Expression> calculate(std::shared_ptr<Call> call, Diff &diff, std::shared_ptr<Diff::DiffContext> context,
                                              std::shared_ptr<Variable> wrt) override;
    };

public:
    explicit DefaultFunctionDiffStorage(std::shared_ptr<Context> context): FunctionDiffStorage(std::move(context)) {
        addDiffCalculator(FunctionSignature("std::cos", Type()), new CosDiffCalculator());
        addDiffCalculator(FunctionSignature("std::sin", Type()), new SinDiffCalculator());
        addDiffCalculator(FunctionSignature("std::pow", Type(), Type()), new PowDiffCalculator());
        addDiffCalculator(FunctionSignature("std::log", Type()), new LogDiffCalculator());
    }
};

#endif //FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H
