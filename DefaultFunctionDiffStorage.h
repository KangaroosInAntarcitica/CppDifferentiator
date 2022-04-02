#ifndef FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H
#define FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H

#include "FunctionDiffStorage.h"

class DefaultFunctionDiffStorage: FunctionDiffStorage {
    static FunctionDiffStorage *instance;

    struct CosDiffCalculator: virtual DiffCalculator {
        Expression *calculate(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) override;
    };

    struct SinDiffCalculator: virtual DiffCalculator {
        Expression *calculate(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) override;
    };

    struct PowDiffCalculator: virtual DiffCalculator {
        Expression *calculate(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) override;
    };

    struct LogDiffCalculator: virtual DiffCalculator {
        Expression *calculate(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) override;
    };

    DefaultFunctionDiffStorage() {
        addTypeConversion("int", "float");
        addTypeConversion("int", "double");
        addTypeConversion("int", "long");
        addTypeConversion("long", "double");
        addTypeConversion("float", "double");
        addTypeConversion("", "double");

        addDiffCalculator(FunctionSignature("std::cos", "double"), new CosDiffCalculator());
        addDiffCalculator(FunctionSignature("std::sin", "double"), new SinDiffCalculator());
        addDiffCalculator(FunctionSignature("std::pow", "double", "double"), new PowDiffCalculator());
        addDiffCalculator(FunctionSignature("std::log", "double"), new LogDiffCalculator());
    }

public:
    static FunctionDiffStorage &singleton() {
        return *instance;
    }
};


#endif //FINAL_PROJECT_DEFAULTFUNCTIONDIFFSTORAGE_H
