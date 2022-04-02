#ifndef FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H
#define FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H

#include <unordered_map>
#include <string>
#include <vector>
#include "SyntaxTreeNode.h"
#include "Diff.h"


struct FunctionSignature {
    std::string name;
    std::vector<std::string> paramTypes;

    FunctionSignature() = default;
    FunctionSignature(const FunctionSignature &o) = default;
    explicit FunctionSignature(std::string name): name(name) {};
    FunctionSignature(std::string name, std::string param): name(name) {
        paramTypes.push_back(param);
    }
    FunctionSignature(std::string name, std::string param1, std::string param2): FunctionSignature(name, param1) {
        paramTypes.push_back(param2);
    }
    explicit FunctionSignature(FunctionDeclaration *decl) {
        name = decl->name;
        for (Variable *var: decl->params) {
            paramTypes.push_back(var->type.name);
        }
    }
    explicit FunctionSignature(FunctionCall *call): FunctionSignature(call->declaration) {}

    bool operator==(const FunctionSignature &o) const {
        return name == o.name && paramTypes == o.paramTypes;
    }
};

namespace std {
    template <> struct hash<FunctionSignature> {
        std::size_t operator()(const FunctionSignature& sign) const;
    };
}

class FunctionDiffStorage {
public:
    struct DiffCalculator {
        virtual Expression *calculate(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) = 0;
    };

protected:
    std::unordered_map<std::string, std::vector<std::string>> typeConversion;
    std::unordered_map<FunctionSignature, std::shared_ptr<DiffCalculator>> functionDiffCalculators;

    std::shared_ptr<DiffCalculator> findDiffCalculatorSearchConversions(FunctionSignature &signature, int paramI=0) {
        std::string paramType = signature.paramTypes[paramI];
        if (!typeConversion.count(paramType)) {
            if (paramI + 1 < signature.paramTypes.size()) {
                return findDiffCalculatorSearchConversions(signature, paramI + 1);
            }
            return nullptr;
        }
        std::vector<std::string> &conversions = typeConversion[signature.paramTypes[paramI]];
        for (std::string &conversion: conversions) {
            signature.paramTypes[paramI] = conversion;
            if (functionDiffCalculators.count(signature)) {
                return functionDiffCalculators[signature];
            }
            std::shared_ptr<DiffCalculator> calculator = findDiffCalculatorSearchConversions(signature, paramI + 1);
            if (calculator != nullptr) {
                return calculator;
            }
        }
        signature.paramTypes[paramI] = paramType;
        return nullptr;
    }

public:
    void addTypeConversion(std::string fromType, std::string toType) {
        if (!typeConversion.count(fromType)) {
            typeConversion[fromType] = std::vector<std::string>();
        }
        typeConversion[fromType].push_back(toType);
    }

    void addDiffCalculator(FunctionSignature signature, DiffCalculator *diffCalculator) {
        functionDiffCalculators[signature] = std::shared_ptr<DiffCalculator>(diffCalculator);
    }

    std::shared_ptr<DiffCalculator> findDiffCalculator(FunctionSignature &signature, int paramI=0) {
        if (functionDiffCalculators.count(signature)) {
            return functionDiffCalculators[signature];
        }
        FunctionSignature copy = signature;
        return findDiffCalculatorSearchConversions(copy);
    }

    Expression *convert(FunctionCall *call, Diff &diff, Diff::DiffContext &context, Variable *wrt) {
        FunctionSignature sign(call);
        std::shared_ptr<DiffCalculator> calc = findDiffCalculator(sign);
        if (calc == nullptr) {
            return nullptr;
        }
        return calc->calculate(call, diff, context, wrt);
    }
};

#endif //FINAL_PROJECT_FUNCTION_DIFF_STORAGE_H
