/** @file main.c
 *
 *  @brief  Lab 5 Fall 2024
 */
#include <349_lib.h>
#include <349_peripheral.h>
#include <349_threads.h>
#include <stdio.h>
#include <pid.h>
#include <stdlib.h>

#define UNUSED __attribute__((unused))

#define B00 0
#define B01 1
#define B10 2
#define B11 3

#define MOTOR_LEFT 0
#define MOTOR_RIGHT 1

#define FREE 0
#define FORWARD 1
#define BACKWARD 2
#define BRAKE 3

#define ENCODER_EN (1 << 1)
#define BUTTON1_EN (1 << 2)
#define BUTTON2_EN (1 << 3)

pid_t pids[2] = {{.k_p = 18, .k_i = 30, .k_d = 1, .k_ff = 0}, {.k_p = 18, .k_i = 30, .k_d = 1, .k_ff = 0}};
int transition_table[4][4] = {{0, 1, 0, -1},
                              {-1, 0, 0, 1},
                              {1, 0, 0, -1},
                              {0, -1, 1, 0}};
double target = 200;

int encoder_right_ticks = 0;
void encoder_right_callback(uint32_t a, uint32_t b) {
    static uint8_t last_state = B00;   
    if (a & ENCODER_EN) {
        uint8_t new_state = ((a & 0x1) << 1) | (b & 0x1);
        encoder_right_ticks -= transition_table[last_state][new_state];
        last_state = new_state;
    } 
    if (a & BUTTON1_EN) {
	if (target < 250) {
            target += 50;
	}
    } 
    if (a & BUTTON2_EN) {
        if (target > 50) {
	    target -= 50;
	}
    }
}

int encoder_left_ticks = 0;
void encoder_left_callback(uint32_t a, uint32_t b) {
    static uint8_t last_christmas = B00;
    uint8_t new_state = (a << 1) | b;
    encoder_left_ticks -= transition_table[last_christmas][new_state];
    last_christmas = new_state;
}

#define PID_PERIOD 60 
double v_avg = 0.0;
double v_left = 0.0;
double v_right = 0.0;
void drive_wheel(void* vargp) {
    (void) vargp;
    v_left = (double)encoder_left_ticks;
    v_right = (double)encoder_right_ticks;
    while(1) {
        v_left = ((double)encoder_left_ticks / PID_PERIOD) * 100;
        v_right = ((double)encoder_right_ticks / PID_PERIOD) * 100;
        v_avg = (v_left + v_right) / 2;
	uint8_t duty1 = pid_effort_tick(&pids[0], target, v_left);
//	uint8_t duty2 = pid_effort_tick(&pids[1], target, v_right);
	motor_set(MOTOR_LEFT, duty1, FORWARD);
	motor_set(MOTOR_RIGHT, duty1, FORWARD);
        encoder_left_ticks = 0;
        encoder_right_ticks = 0;
        wait_until_next_period();
    }
}

#define UART_PERIOD 250
void uart_thread(void *vargp) {
    (void) vargp;
    while(1) {
    	printf("%d,%d\n", (int)v_left, (int)target);
	wait_until_next_period();
    }
}

# define LCD_PERIOD 1500 
void lcd_thread(void* vargp) {
    (void) vargp;
    lcd_init();
    lcd_clear_scr();

    char buffer1[16];
    char buffer2[16];
    while(1) {
	int n = 0;
        n = sprintf(buffer1, "Motor1 E: %d", (int)(target - v_left));
        for (int i = n; i < 16; i++) {
            buffer1[i] = ' ';	
	}
	n = sprintf(buffer2, "%d %d %d", 
		(int)(pids[0].k_p), 
		(int)(pids[0].k_i), (int)(pids[0].k_d));
	for (int i = n; i < 16; i++) {
	    buffer2[i] = ' ';
	}
	lcd_set_cursor(0, 0);
	lcd_print(buffer1);
	lcd_set_cursor(1, 0);
	lcd_print(buffer2);
	wait_until_next_period();

        n = sprintf(buffer1, "Motor2 E: %d", (int)(target - v_left));
	for (int i = n; i < 16; i++) {
	    buffer1[i] = ' ';
	}
        n = sprintf(buffer2, "%d %d %d", 
		(int)(pids[1].k_p), (int)(pids[1].k_i), (int)(pids[1].k_d));
	for (int i = n; i < 16; i++) {
	    buffer2[i] = ' ';
	}
	lcd_set_cursor(0, 0);
	lcd_print(buffer1);
	lcd_set_cursor(1, 0);
	lcd_print(buffer2);
	wait_until_next_period();
    }
}


int main(UNUSED int argc, UNUSED char const *argv[])
{
    thread_init(3, 256, NULL, 1);
    register_encoder_callback(0, &encoder_left_callback);
    register_encoder_callback(1, &encoder_right_callback);

    ABORT_ON_ERROR (thread_create(&drive_wheel, 0, 10, PID_PERIOD, NULL));
    ABORT_ON_ERROR (thread_create(&uart_thread, 1, 100, UART_PERIOD, NULL));
    ABORT_ON_ERROR (thread_create(&lcd_thread, 2, 250, LCD_PERIOD, NULL));

    pid_init(&pids[0], 1);
    pid_init(&pids[1], 1);
    scheduler_start(1000); // every 10ms
    
    while(1) {

    }
    return RET_0349;
}
