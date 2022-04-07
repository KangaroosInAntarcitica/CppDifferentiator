#include <cmath>

#include <vector>

float d_function(float x) {
	return 1;
}

float d_sum(std::vector<float> x) {
	float d_x_result = 0;
	float result = 0;
	int d_x_i = 0;
	for (int i = 0; i < x.size(); ++i) {
		d_x_result += 1[i];
		result += x[i];
	}
	return d_x_result;
}
