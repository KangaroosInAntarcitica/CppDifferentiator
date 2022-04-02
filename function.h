#include <cmath>

double function2 (float x) {
    double a = std::pow(x, 2);
    if (a > 1) {
        return 0;
    } else if (a == 2 || a == 3) {
        a = a * x;
        return a * x;
    } else {
        int i = 0;
        while (i < 5) {
            a += a * i;
            i = i + 1;
        }
        return a;
    }
}

float function (float x) {
    return std::pow(x, 3);
}

float function3 (float x) {
    return x * x * x * x;
}

double function4(double x) {
    return std::sin(x) * std::pow(x, 3);
}
