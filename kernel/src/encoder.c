#include <arm.h>
#include <encoder.h>
#include <exti.h>
#include <gpio.h>
#include <nvic.h>
#include <printk.h>
#include <stdint.h>
#include <systick.h>
#include <unistd.h>

#define NUM_ENCODERS UINT32_C(2)
#define PIN_NUM(side, encoder) \
    encoder_attributes[(side)].pins.encoder_pin_##encoder.num
#define GPIO_PIN(side, encoder)                                 \
    encoder_attributes[(side)].pins.encoder_pin_##encoder.port, \
        encoder_attributes[(side)].pins.encoder_pin_##encoder.num

struct encoder_attr
{
    void (*encoder_isr_callback)(uint32_t a, uint32_t b);
    struct encoder_pin_attr pins;
};

static struct encoder_attr encoder_attributes[NUM_ENCODERS] = {
    {.encoder_isr_callback = NULL}, // Left encoder
    {.encoder_isr_callback = NULL}  // Right encoder
};

void encoder_init(enum encoder_mapping encoder, struct encoder_pin_attr *attr)
{
    // Save any pin configurations as they are needed in the internal operation
    // of the module
    encoder_attributes[encoder].pins = *attr;

    // Initialize GPIOs to be MODE_INPUT, PUSH_PULL, HIGH_SPEED, NO PULL, ALT0
    gpio_init(attr->encoder_pin_a.port, attr->encoder_pin_a.num, MODE_INPUT,
              OUTPUT_PUSH_PULL, OUTPUT_SPEED_HIGH, PUPD_NONE, ALT0);
    gpio_init(attr->encoder_pin_b.port, attr->encoder_pin_b.num, MODE_INPUT,
              OUTPUT_PUSH_PULL, OUTPUT_SPEED_HIGH, PUPD_NONE, ALT0);

    // Configure external interrupts to be both rising and falling edge
    enable_exti(attr->encoder_pin_a.port, attr->encoder_pin_a.num,
                RISING_FALLING_EDGE);
    enable_exti(attr->encoder_pin_b.port, attr->encoder_pin_b.num,
                RISING_FALLING_EDGE);

    // Enable appropriate interrupt numbers
    nvic_irq(attr->encoder_pin_a.irq_num, IRQ_ENABLE);
    nvic_irq(attr->encoder_pin_b.irq_num, IRQ_ENABLE);
}

void encoder_stop(enum encoder_mapping encoder)
{
    struct encoder_attr *attr = &encoder_attributes[encoder];
    disable_exti(attr -> pins.encoder_pin_a.num);
    disable_exti(attr -> pins.encoder_pin_b.num);

    nvic_irq(attr->pins.encoder_pin_a.irq_num, IRQ_DISABLE);
    nvic_irq(attr->pins.encoder_pin_b.irq_num, IRQ_DISABLE);
}

/**
 * Note: DO NOT MODIFY THIS FUNCTION
 */
int sys_register_encoder_callback(uint32_t encoder,
                                  void (*callback)(uint32_t, uint32_t))
{
    (void)encoder;
    (void)callback;
    // NOTE: There are a few ways to implement callback registration. The
    // given method takes in 2 parameters with the first being the encoder
    // to register the callback to, and the second being the callback.
    if (encoder >= NUM_ENCODERS)
    {
        return -1;
    }

    encoder_attributes[encoder].encoder_isr_callback = callback;

    // NOTE: Return -1 if encoder input is out of range
    return 0;
}

// NOTE: Make any modifications to the irq handlers if needed. This is where
// your PCB design decisions may make your life either easier or harder!
void encoder_irq_handler_left() {
    uint32_t temp_a = gpio_read(encoder_attributes[ENCODER_LEFT].pins.encoder_pin_a.port,
		                encoder_attributes[ENCODER_LEFT].pins.encoder_pin_a.num);
    uint32_t temp_b = gpio_read(encoder_attributes[ENCODER_LEFT].pins.encoder_pin_b.port,
		                encoder_attributes[ENCODER_LEFT].pins.encoder_pin_b.num);
    encoder_attributes[ENCODER_LEFT].encoder_isr_callback(temp_a, temp_b);
    exti_clear_pending_bit(0);
    exti_clear_pending_bit(1);
    return;
}

#define ENCODER_EN (1 << 1)
#define BUTTON1_EN (1 << 2)
#define BUTTON2_EN (1 << 3)

#define DEBOUNCE_TIME 10
static uint32_t last_pr8 = 0;
static uint32_t last_pr9 = 0;

void encoder_irq_handler_right(){
    struct exti* e = EXTI_BASE;
    uint8_t a = 0;
    uint8_t b = 0;
    if (e -> pr & (1 << 6) || e -> pr & (1 << 7)) {
        uint32_t temp_a = gpio_read(encoder_attributes[ENCODER_RIGHT].pins.encoder_pin_a.port,
	    	                    encoder_attributes[ENCODER_RIGHT].pins.encoder_pin_a.num);
        uint32_t temp_b = gpio_read(encoder_attributes[ENCODER_RIGHT].pins.encoder_pin_b.port,
		                    encoder_attributes[ENCODER_RIGHT].pins.encoder_pin_b.num);
        a = temp_a | ENCODER_EN;
        b = temp_b;
	exti_clear_pending_bit(6);
        exti_clear_pending_bit(7);
    }
    if(e -> pr & (1 << 8)) {
	if ((systick_get_ticks() - last_pr8) > DEBOUNCE_TIME) {
	    a |= BUTTON1_EN;
	    last_pr8 = systick_get_ticks();
	}
        exti_clear_pending_bit(8); 
    }
    if (e -> pr & (1 << 9)) {
	if ((systick_get_ticks() - last_pr9) > DEBOUNCE_TIME) {
	    a |= BUTTON2_EN;
	    last_pr9 = systick_get_ticks();
	}
        exti_clear_pending_bit(9); 
    }
    encoder_attributes[ENCODER_RIGHT].encoder_isr_callback(a, b);
    return;
}
