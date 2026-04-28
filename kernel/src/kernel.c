/**
 * @file     kernel.c
 *
 * @brief    Kernel entry point
 *
 */

#include "kernel.h"
#include "arm.h"
#include "motor_driver.h"
#include <uart.h>
#include <pwm.h>
#include <exti.h>
#include <lcd_driver.h>
#include "systick.h"
#include "spi.h"
#include "printk.h"

#define UART_USARTDIV_MANTISSA 8
#define UART_USARTDIV_FRAC 11
#define UART_USARTDIV (UART_USARTDIV_MANTISSA << 4) | (UART_USARTDIV_FRAC & 0xF)

#define RISING_EDGE 1
#define FALLING_EDGE 2
#define RISING_FALLING_EDGE 3

#define MOTORA_EN_PORT GPIO_B
#define MOTORA_EN_NUM 10
#define ENCB_L_PORT GPIO_A
#define ENCB_L_NUM 0
#define ENCA_L_PORT GPIO_A
#define ENCA_L_NUM 1
#define MOTORA_IN1_PORT GPIO_C
#define MOTORA_IN1_NUM 0
#define MOTORA_IN2_PORT GPIO_C
#define MOTORA_IN2_NUM 1

#define MOTORB_EN_PORT GPIO_B
#define MOTORB_EN_NUM 0
#define ENCB_R_PORT GPIO_A
#define ENCB_R_NUM 6
#define ENCA_R_PORT GPIO_A
#define ENCA_R_NUM 7
#define MOTORB_IN1_PORT GPIO_B
#define MOTORB_IN1_NUM 4
#define MOTORB_IN2_PORT GPIO_B
#define MOTORB_IN2_NUM 5

#define LED1_PORT GPIO_A
#define LED1_NUM 5
#define LED2_PORT GPIO_A
#define LED2_NUM 10

#define BUTTON1_PORT GPIO_A
#define BUTTON1_NUM 8
#define BUTTON2_PORT GPIO_A
#define BUTTON2_NUM 9

#define SCL_PORT GPIO_B
#define SCL_NUM 8
#define SDA_PORT GPIO_B
#define SDA_NUM 9

#define SERVO_PORT GPIO_B
#define SERVO_NUM 6

#define MOTORA_TIMER 2
#define MOTORA_CHN 3
#define MOTORA_ALT ALT1

#define MOTORB_TIMER 3
#define MOTORB_CHN 3
#define MOTORB_ALT ALT2

#define EXTI0_IRQ 6
#define EXTI1_IRQ 7
#define EXTI2_IRQ 8
#define EXTI3_IRQ 9
#define EXTI4_IRQ 10
#define EXTI9_5_IRQ 23
#define EXTI15_10_IRQ 40

int kernel_main(void) {
    init_349(); // DO NOT REMOVE THIS LINE
    uart_init(UART_USARTDIV);
    enable_fpu();
    // struct pin left_motor_in1 = {.port=MOTORA_IN1_PORT, .num=MOTORA_IN1_NUM, .irq_num=0};
    // struct pin left_motor_in2 = {.port=MOTORA_IN2_PORT, .num=MOTORA_IN2_NUM, .irq_num=0};
    // struct pin left_motor_en = {.port=MOTORA_EN_PORT, .num=MOTORA_EN_NUM, .irq_num=0};
    // struct motor_timer left_timer = {.timer=MOTORA_TIMER, .channel=MOTORA_CHN, 
	//                              .gpio_alt=MOTORA_ALT, .is_comp=0};
    // struct motor_attr left_motor = {.motor_in1=left_motor_in1, .motor_in2=left_motor_in2,
    //                                 .motor_en=left_motor_en, .timer=left_timer};

    // struct pin right_motor_in1 = {.port=MOTORB_IN1_PORT, .num=MOTORB_IN1_NUM, .irq_num=0};
    // struct pin right_motor_in2 = {.port=MOTORB_IN2_PORT, .num=MOTORB_IN2_NUM, .irq_num=0};
    // struct pin right_motor_en = {.port=MOTORB_EN_PORT, .num=MOTORB_EN_NUM, .irq_num=0};
    // struct motor_timer right_timer = {.timer=MOTORB_TIMER, .channel=MOTORB_CHN, 
	//                              .gpio_alt=MOTORB_ALT, .is_comp=0};
    // struct motor_attr right_motor = {.motor_in1=right_motor_in1, .motor_in2=right_motor_in2,
    //                                 .motor_en=right_motor_en, .timer=right_timer};

    // struct pin left_encoder_a = {.port=ENCA_L_PORT, .num=ENCA_L_NUM, .irq_num=EXTI1_IRQ};
    // struct pin left_encoder_b = {.port=ENCB_L_PORT, .num=ENCB_L_NUM, .irq_num=EXTI0_IRQ};
    // struct pin right_encoder_a = {.port=ENCA_R_PORT, .num=ENCA_R_NUM, .irq_num=EXTI9_5_IRQ};
    // struct pin right_encoder_b = {.port=ENCB_R_PORT, .num=ENCB_R_NUM, .irq_num=EXTI9_5_IRQ};
    // struct encoder_pin_attr left_encoder = {.encoder_pin_a=left_encoder_a,
    //                                         .encoder_pin_b=left_encoder_b};
    // struct encoder_pin_attr right_encoder = {.encoder_pin_a=right_encoder_a,
    //                                          .encoder_pin_b=right_encoder_b};

    // motor_init(LEFT_MOTOR, &left_motor, &left_encoder);
    // motor_init(RIGHT_MOTOR, &right_motor, &right_encoder);

    // gpio_init(BUTTON1_PORT, BUTTON1_NUM, MODE_INPUT, OUTPUT_PUSH_PULL, 
	//       OUTPUT_SPEED_HIGH, PUPD_PULL_UP, ALT0);
    // enable_exti(BUTTON1_PORT, BUTTON1_NUM, RISING_EDGE);
    // gpio_init(BUTTON2_PORT, BUTTON2_NUM, MODE_INPUT, OUTPUT_PUSH_PULL, 
	//       OUTPUT_SPEED_HIGH, PUPD_PULL_UP, ALT0);
    // enable_exti(BUTTON2_PORT, BUTTON2_NUM, RISING_EDGE);
    
    enter_user_mode();

    // sys_spi_init(LINK_IN);
    // sys_spi_init(LINK_OUT);

    // char *s = "hello";

    while (1) {
    //     sys_spi_transmit((uint8_t*)s, 5);
    //     for (int i = 0; i < 10000; i++);
    }
    return 0;
}
