#include <cmath>
#include <vector>
#include <array>


std::array<double, 4> system(double x1, double x2, double x3, double u) {
    std::array<double, 4> result;
    result[0] = x2 + std::pow(x3, 2);
    result[1] = (1 - 2 * x3) * u + std::sin(x1) - x2 + x3 - x3 * x3;
    result[2] = u;
    result[3] = x1;
    return result;
}

// x and y, velocity x and y, angle theta, angular velocity, acceleration, angular acceleration
std::array<double, 6> spaceVehicleSystem(double x, double y, double vx, double vy, double theta, double vTheta,
                                         double a, double aTheta) {
    std::array<double, 6> result;
    result[0] = vx;
    result[1] = vy;
    result[2] = std::cos(theta) * a;
    result[3] = std::sin(theta) * a;
    result[4] = vTheta;
    result[5] = aTheta;
    return result;
}


std::vector<double> pendulumSystem(double theta, double dTheta) {
    std::vector<double> result(2, 0);
    result[0] = dTheta;
    result[1] = 10 - std::sin(theta);
    return result;
}

double func2(double input) {
    double a = std::exp(2) + std::abs(input);
    return std::pow(input, 10) * a;
}

