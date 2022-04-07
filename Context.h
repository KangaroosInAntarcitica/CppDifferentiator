#ifndef FINAL_PROJECT_CONTEXT_H
#define FINAL_PROJECT_CONTEXT_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <sstream>


struct Variable;
struct FunctionDeclaration;

struct Type {
    std::string name;
    bool isGeneric = false;
    std::vector<Type> generics;

    explicit Type(std::string name): name(std::move(name)) {}
    Type(std::string name, std::vector<Type> generics):
            name(std::move(name)), generics(std::move(generics)) {}
    Type(): isGeneric(true) {}

    std::string to_string() {
        if (isGeneric) {
            return "?";
        }

        std::string result = name;
        if (!generics.empty()) {
            result += '<';
            for (int i = 0; i < generics.size(); ++i) {
                if (i != 0) result += ", ";
                result += generics[i].to_string();
            }
            result += '>';
        }
        return result;
    };

    bool operator==(const Type &o) const {
        return name == o.name && isGeneric == o.isGeneric && generics == o.generics;
    }
};


struct FunctionSignature {
    std::string name;
    std::vector<Type> paramTypes;

    FunctionSignature() = default;
    FunctionSignature(const FunctionSignature &o) = default;
    explicit FunctionSignature(std::string name): name(std::move(name)) {};
    FunctionSignature(std::string name, Type param): name(std::move(name)) {
        paramTypes.push_back(std::move(param));
    }
    FunctionSignature(std::string name, Type param1, Type param2):
            FunctionSignature(std::move(name), std::move(param1)) {
        paramTypes.push_back(std::move(param2));
    }
    FunctionSignature(std::string name, std::vector<Type> paramTypes): name(name), paramTypes(paramTypes) {}

    bool operator==(const FunctionSignature &o) const {
        return name == o.name && paramTypes == o.paramTypes;
    }

    std::string to_string() {
        std::ostringstream result;
        result << name << "(";
        for (Type &type: paramTypes) {
            result << type.to_string();
        }
        result << ")";
        return result.str();
    }
};

namespace std {
    template <> struct hash<Type> {
        std::size_t operator()(const Type& type) const;
    };

    template <> struct hash<FunctionSignature> {
        std::size_t operator()(const FunctionSignature& sign) const;
    };
}

struct Context {
public:
    std::unordered_map<std::string, Type> types;
    std::unordered_map<std::string, std::shared_ptr<Variable>> variables;
    std::unordered_set<FunctionSignature> functions;
    std::unordered_map<Type, std::vector<Type>> typeConversions;
    std::shared_ptr<Context> parent = nullptr;

    Context() = default;
    explicit Context(std::shared_ptr<Context> parent): parent(std::move(parent)) {}

    void addType(const std::string &name, Type type) {
        types[name] = std::move(type);
    }

    void addVariable(const std::string &name, std::shared_ptr<Variable> var) {
        variables[name] = std::move(var);
    }

    void addFunction(const FunctionSignature &sign) {
        functions.insert(std::move(sign));
    }

    void addTypeConversion(const Type &typeFrom, const Type &typeTo) {
        if (!typeConversions.count(typeFrom)) {
            typeConversions[typeFrom] = std::vector<Type>();
        }
        typeConversions[typeFrom].push_back(typeTo);
    }

    bool isVariablePresent(const std::string& name) {
        return variables.count(name) ||
               (parent != nullptr && parent->isVariablePresent(name));
    }

    bool isTypePresent(std::string& name) {
        return types.count(name) ||
               (parent != nullptr && parent->isTypePresent(name));
    }

    std::shared_ptr<Variable> getVariable(std::string& name) {
        return variables.count(name) > 0 ? variables[name] :
               (parent == nullptr ? nullptr : parent->getVariable(name));
    }

    Type getType(std::string& name) {
        return types.count(name) > 0 ? types[name] :
               (parent == nullptr ? Type() : parent->getType(name));
    }

protected:
    std::unique_ptr<FunctionSignature> findExactDefinedFunction(FunctionSignature &desired) {
        if (functions.count(desired)) {
            return std::make_unique<FunctionSignature>(desired);
        }
        return parent == nullptr ? nullptr : parent->findExactDefinedFunction(desired);
    }

    std::unique_ptr<FunctionSignature> findDefinedFunctionSearchConversions(FunctionSignature &desired, int paramI=0) {
        Type paramType = desired.paramTypes[paramI];

        // First try generic type
        Type genericType;
        desired.paramTypes[paramI] = genericType;
        std::unique_ptr<FunctionSignature> signature = findExactDefinedFunction(desired);
        if (signature != nullptr) {
            return signature;
        }
        desired.paramTypes[paramI] = paramType;

        // Try the conversions
        if (typeConversions.count(paramType)) {
            std::vector<Type> &conversions = typeConversions[desired.paramTypes[paramI]];
            for (Type &conversion: conversions) {
                desired.paramTypes[paramI] = conversion;
                signature = findExactDefinedFunction(desired);
                if (signature != nullptr) {
                    return signature;
                }
                signature = findDefinedFunctionSearchConversions(desired, paramI + 1);
                if (signature != nullptr) {
                    return signature;
                }
            }
            desired.paramTypes[paramI] = paramType;
        }

        // End
        if (paramI + 1 < desired.paramTypes.size()) {
            return findDefinedFunctionSearchConversions(desired, paramI + 1);
        }
        return nullptr;
    }

public:
    std::unique_ptr<FunctionSignature> findFunction(FunctionSignature &desired) {
        std::unique_ptr<FunctionSignature> signature = findExactDefinedFunction(desired);
        if (signature != nullptr) {
            return signature;
        }
        if (!desired.paramTypes.empty()) {
            FunctionSignature copy = desired;
            return findDefinedFunctionSearchConversions(copy, 0);
        }
        return signature;
    }

    Context *copy() {
        auto *context = new Context();
        context->parent = parent;
        context->types = types;
        context->variables = variables;
        context->functions = functions;
        context->typeConversions = typeConversions;
        return context;
    }
};

#endif //FINAL_PROJECT_CONTEXT_H
