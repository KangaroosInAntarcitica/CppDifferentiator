#include <array>
#include <cmath>
#include <vector>

std::array<std::array<double, 4>, 4> d_system(double x1, double x2, double x3, double u) {
	std::array<double, 4> d_x1_result;
	std::array<double, 4> d_x2_result;
	std::array<double, 4> d_x3_result;
	std::array<double, 4> d_u_result;
	std::array<double, 4> result;
	d_x1_result[0] = 0;
	d_x2_result[0] = 1;
	d_x3_result[0] = 2 * std::pow(x3, 1);
	d_u_result[0] = 0;
	result[0] = x2 + std::pow(x3, 2);
	d_x1_result[1] = (0) * u + std::cos(x1);
	d_x2_result[1] = (0) * u + 1;
	d_x3_result[1] = (2) * u + 1 - x3 + x3;
	d_u_result[1] = (0) * u + (1 - 2 * x3);
	result[1] = (1 - 2 * x3) * u + std::sin(x1) - x2 + x3 - x3 * x3;
	d_x1_result[2] = 0;
	d_x2_result[2] = 0;
	d_x3_result[2] = 0;
	d_u_result[2] = 1;
	result[2] = u;
	d_x1_result[3] = 1;
	d_x2_result[3] = 0;
	d_x3_result[3] = 0;
	d_u_result[3] = 0;
	result[3] = x1;
	std::array<std::array<double, 4>, 4> _return;
	_return[0] = d_x1_result;
	_return[1] = d_x2_result;
	_return[2] = d_x3_result;
	_return[3] = d_u_result;
	return _return;
}
// x and y, velocity x and y, angle theta, angular velocity, acceleration, angular acceleration

std::array<std::array<double, 6>, 8> d_spaceVehicleSystem(double x, double y, double vx, double vy, double theta, double vTheta, double a, double aTheta) {
	std::array<double, 6> d_x_result;
	std::array<double, 6> d_y_result;
	std::array<double, 6> d_vx_result;
	std::array<double, 6> d_vy_result;
	std::array<double, 6> d_theta_result;
	std::array<double, 6> d_vTheta_result;
	std::array<double, 6> d_a_result;
	std::array<double, 6> d_aTheta_result;
	std::array<double, 6> result;
	d_x_result[0] = 0;
	d_y_result[0] = 0;
	d_vx_result[0] = 1;
	d_vy_result[0] = 0;
	d_theta_result[0] = 0;
	d_vTheta_result[0] = 0;
	d_a_result[0] = 0;
	d_aTheta_result[0] = 0;
	result[0] = vx;
	d_x_result[1] = 0;
	d_y_result[1] = 0;
	d_vx_result[1] = 0;
	d_vy_result[1] = 1;
	d_theta_result[1] = 0;
	d_vTheta_result[1] = 0;
	d_a_result[1] = 0;
	d_aTheta_result[1] = 0;
	result[1] = vy;
	d_x_result[2] = 0;
	d_y_result[2] = 0;
	d_vx_result[2] = 0;
	d_vy_result[2] = 0;
	d_theta_result[2] = -std::sin(theta) * a;
	d_vTheta_result[2] = 0;
	d_a_result[2] = std::cos(theta);
	d_aTheta_result[2] = 0;
	result[2] = std::cos(theta) * a;
	d_x_result[3] = 0;
	d_y_result[3] = 0;
	d_vx_result[3] = 0;
	d_vy_result[3] = 0;
	d_theta_result[3] = std::cos(theta) * a;
	d_vTheta_result[3] = 0;
	d_a_result[3] = std::sin(theta);
	d_aTheta_result[3] = 0;
	result[3] = std::sin(theta) * a;
	d_x_result[4] = 0;
	d_y_result[4] = 0;
	d_vx_result[4] = 0;
	d_vy_result[4] = 0;
	d_theta_result[4] = 0;
	d_vTheta_result[4] = 1;
	d_a_result[4] = 0;
	d_aTheta_result[4] = 0;
	result[4] = vTheta;
	d_x_result[5] = 0;
	d_y_result[5] = 0;
	d_vx_result[5] = 0;
	d_vy_result[5] = 0;
	d_theta_result[5] = 0;
	d_vTheta_result[5] = 0;
	d_a_result[5] = 0;
	d_aTheta_result[5] = 1;
	result[5] = aTheta;
	std::array<std::array<double, 6>, 8> _return;
	_return[0] = d_x_result;
	_return[1] = d_y_result;
	_return[2] = d_vx_result;
	_return[3] = d_vy_result;
	_return[4] = d_theta_result;
	_return[5] = d_vTheta_result;
	_return[6] = d_a_result;
	_return[7] = d_aTheta_result;
	return _return;
}

std::array<std::vector<double>, 2> d_pendulumSystem(double theta, double dTheta) {
	std::vector<double> d_theta_result(2, 0);
	std::vector<double> d_dTheta_result(2, 0);
	std::vector<double> result(2, 0);
	d_theta_result[0] = 0;
	d_dTheta_result[0] = 1;
	result[0] = dTheta;
	d_theta_result[1] = std::cos(theta);
	d_dTheta_result[1] = 0;
	result[1] = 10 - std::sin(theta);
	std::array<std::vector<double>, 2> _return;
	_return[0] = d_theta_result;
	_return[1] = d_dTheta_result;
	return _return;
}

double d_func2(double input) {
	double d_input_a = (input > 0) - (input < 0);
	double a = std::exp(2) + std::abs(input);
	return 10 * std::pow(input, 9) * a + std::pow(input, 10) * d_input_a;
}
