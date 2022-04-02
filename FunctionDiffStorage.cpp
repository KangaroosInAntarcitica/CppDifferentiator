#include "FunctionDiffStorage.h"

namespace std {
    std::size_t hash<FunctionSignature>::operator()(const FunctionSignature &sign) const {
        size_t result = sign.paramTypes.size();
        for(const std::string &type: sign.paramTypes) {
            result ^= std::hash<std::string>()(type) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }
        return std::hash<std::string>()(sign.name) ^ (result << 6);
    }
}
