#include <math.h>
#include <pid.h>
#include <stdint.h>

#define MIN_EFFORT 0
#define MAX_EFFORT 255
#define ERROR_PRESCALE 0.05
#define I_PRESCALE 0.1
#define FF_PRESCALE 0.1
#define I_SCALE 0.1

static char ff_enabled = 1; // PID feedforward enabled

double clamp(double min, double v, double max) {
	if (v <= min) return min;
	if (v >= max) return max;
	return v;
}

void pid_init(pid_t *pid, char enable_ff) {
	pid->accumulator = 0;
	pid->prev_error = 0;
	ff_enabled = enable_ff;
	pid->integrator_enabled = 1;
}

uint8_t pid_effort_tick(pid_t *pid, double target, double measured) {
	double error = target - measured;
	error *= ERROR_PRESCALE;
	double p_contrib = pid->k_p * error;

	pid->accumulator += error;
	double i_contrib;
	double diff = ((error / ERROR_PRESCALE) / target) * 100;
	if (diff > 80.0 || diff < -80.0) {
	    pid_reset_integrator(pid);
	}
	i_contrib = I_PRESCALE * pid->k_i * pid->accumulator;

	double error_dt = error - pid->prev_error;
	double d_contrib = pid->k_d * error_dt;
	
	pid->prev_error = error;

	double ff_contrib;
	if (ff_enabled) ff_contrib = FF_PRESCALE * pid->k_ff * target;
	else ff_contrib = 0;

	double raw_effort = p_contrib + i_contrib + d_contrib + ff_contrib;
	double clamped_effort = clamp(MIN_EFFORT, raw_effort, MAX_EFFORT); 

	if (clamped_effort != raw_effort && pid->integrator_enabled) {// motor is saturated
		pid->integrator_enabled = 0; }
	else if (clamped_effort == raw_effort && !(pid->integrator_enabled))
		pid->integrator_enabled = 1;

	uint8_t pwm = (uint8_t) clamped_effort;
	return pwm;
}

void pid_reset_integrator(pid_t *pid) {
	pid->accumulator = 0;
	pid->integrator_enabled = 1;
}
