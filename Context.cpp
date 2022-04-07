#include "Context.h"

namespace std {
    std::size_t hash<Type>::operator()(const Type &type) const {
        size_t result = type.generics.size();
        for(const Type &generic: type.generics) {
            result ^= std::hash<Type>()(generic) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return (std::hash<std::string>()(type.name) ^ (result << 6)) << 6 ^ (type.isGeneric ? 53 : 83);
    }

    std::size_t hash<FunctionSignature>::operator()(const FunctionSignature &sign) const {
        size_t result = sign.paramTypes.size();
        for(const Type &type: sign.paramTypes) {
            result ^= std::hash<Type>()(type) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return std::hash<std::string>()(sign.name) ^ (result << 6);
    }
}
