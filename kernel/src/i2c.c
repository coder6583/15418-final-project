/* ************************************ */
/* @brief I2C implementation in C. Only implements write. It uses polling, a very naive
 * method.
 * 
 * @date 10/2/2025
 *
 * @author Soma Narita
 */
#include <unistd.h>

#include <gpio.h>
#include <i2c.h>
#include <rcc.h>

struct i2c_reg_map {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
};

#define I2C1_BASE (struct i2c_reg_map *) 0x40005400

#define I2C_PER_FREQ 0x10
#define I2C_CCR 0x50
#define I2C_EN 0x1

#define I2C_START (1 << 8)
#define I2C_STOP (1 << 9)
#define I2C_SR1_SB (1)
#define I2C_SR1_ADDR (1 << 1)
#define I2C_SR1_TxE (1 << 7)
#define I2C_SR1_BTF (1 << 2)

#define I2C_CLOCK_EN (1 << 21)
#define GPIOB_CLOCK_EN (1 << 1)

// init the master
void i2c_master_init(uint16_t clk){
    struct rcc_reg_map *rcc = RCC_BASE;
    struct i2c_reg_map *i2c = I2C1_BASE;

    rcc -> apb1_enr |= I2C_CLOCK_EN;
    rcc -> ahb1_enr |= GPIOB_CLOCK_EN;

    gpio_init(GPIO_B, 8, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT4);
    gpio_init(GPIO_B, 9, MODE_ALT, OUTPUT_OPEN_DRAIN, OUTPUT_SPEED_LOW, PUPD_NONE, ALT4);

    i2c -> CR2 |= I2C_PER_FREQ;
    i2c -> CCR |= clk;
    // i2c -> CCR = I2C_CCR; //0d80 * 62.5ns = 5000ns = T_high

    i2c -> CR1 |= I2C_EN;
    return;
}

// start the master
void i2c_master_start() {
    struct i2c_reg_map *i2c = I2C1_BASE;
    i2c -> CR1 |= I2C_START;
    
    while(!(i2c -> SR1 & I2C_SR1_SB));
    return;
}


// stop the master
void i2c_master_stop() {
    struct i2c_reg_map *i2c = I2C1_BASE;
    i2c -> CR1 |= I2C_STOP;
    return;
}

// i2c write to a slave addr as a master
int i2c_master_write(uint8_t *buf, uint16_t len, uint8_t slave_addr){
    struct i2c_reg_map *i2c = I2C1_BASE;

    i2c -> DR = (slave_addr << 1) | 0x00;
    while(!(i2c -> SR1 & I2C_SR1_ADDR));
    (void)i2c -> SR2;

    for (int i = 0; i < len; i++) {
        while (!(i2c -> SR1 & I2C_SR1_TxE));
        i2c -> DR = buf[i]; 
    }

    while (!(i2c -> SR1 & I2C_SR1_TxE) || !(i2c -> SR2 & I2C_SR1_BTF));
    return 0;
}

// i2c master read (NOT IMPLEMENTED YET)
int i2c_master_read(uint8_t *buf, uint16_t len, uint8_t slave_addr){
    (void) buf;
    (void) len;
    (void) slave_addr;

    return 0;
}
