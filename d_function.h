#include <cmath>

double d_function2(float x) {
	double d_x_a = 2 * std::pow(x, 2 - 1) * 1 + std::pow(x, 2) * std::log(x) * 0;
	double a = std::pow(x, 2);
	if (a > 1) {
		return 0;
	} else if (a == 2 || a == 3) {
		d_x_a = d_x_a * x + a * 1;
		a = a * x;
		return d_x_a * x + a * 1;
	} else {
		int d_x_i = 0;
		int i = 0;
		while (i < 5) {
			d_x_a += d_x_a * i + a * d_x_i;
			a += a * i;
			d_x_i = d_x_i + 0;
			i = i + 1;
		}
		return d_x_a;
	}
}

float d_function(float x) {
	return 3 * std::pow(x, 3 - 1) * 1 + std::pow(x, 3) * std::log(x) * 0;
}

float d_function3(float x) {
	return 1 * x * x * x + x * (1 * x * x + x * (1 * x + x * 1));
}

double d_function4(double x) {
	return std::cos(x) * 1 * std::pow(x, 3) + std::sin(x) * (3 * std::pow(x, 3 - 1) * 1 + std::pow(x, 3) * std::log(x) * 0);
}
