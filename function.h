#include <cmath>
#include <vector>

float function(float x) {
    return x;
}

float sum(std::vector<float> x) {
    float result = 0;

    for (int i = 0; i < x.size(); ++i) {
        result += x[i];
    }

    return result;
}
