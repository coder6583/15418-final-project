#include <stdint.h>

typedef struct {
	double accumulator; 
	double prev_error;
	// TUNE THESE
	uint32_t k_p;
	uint32_t k_i;
	uint32_t k_d;
	uint32_t k_ff;
	char integrator_enabled;
} pid_t;


// initialize the pid system
void pid_init(pid_t *pid, char enable_ff);

// call this on every tick, result \in [0, 255] \ N
uint8_t pid_effort_tick(pid_t *pid, double target, double measured);

// reset the I term. CALL THIS EVERY TIME YOU CHANGE THE TARGET
void pid_reset_integrator(pid_t *pid);
