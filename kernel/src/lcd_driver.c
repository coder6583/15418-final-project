#include <unistd.h>

#include <i2c.h>
#include <lcd_driver.h>
#include <systick.h>

#define LCD_ADDR (0x4E >> 1)
#define CLOCK_MHZ 84

#define MSB4_MASK 0xF0
#define LSB4_MASK 0x0F

#define E_HIGH 0x0C
#define E_LOW 0x08

#define CLK_16MHZ 0x50

#define INIT_SEQ1 0x3C
#define INIT_SEQ2 0x38

#define SET_4BIT_MODE1 0x2C
#define SET_4BIT_MODE2 0x28

#define LCD_FUNCSET 0x2C
#define LCD_DISPOFF 0x08
#define LCD_CLEAR 0x01
#define LCD_ENTRYMODE 0x06
#define LCD_DISPON 0x0C
#define LCD_CURSOR 0x80

#define CHAR_E_HIGH 0x0D
#define CHAR_E_LOW 0x09

// send a command to the LCD screen
void lcd_send_cmd(uint8_t cmd) {
    uint8_t data[] = {(cmd & MSB4_MASK) | E_HIGH, (cmd & MSB4_MASK) | E_LOW,
                      ((cmd & LSB4_MASK) << 4) | E_HIGH, ((cmd & LSB4_MASK) << 4) | E_LOW};
    i2c_master_start();
    i2c_master_write(data, 4, LCD_ADDR);
    i2c_master_stop();
    return;
}

// initialize the lcd driver
void lcd_driver_init() {
    i2c_master_init(CLK_16MHZ);
    
    systick_delay(30);

    uint8_t init1[] = {INIT_SEQ1, INIT_SEQ2};
    i2c_master_start();
    i2c_master_write(init1, 2, LCD_ADDR);
    i2c_master_stop();

    systick_delay(5);

    uint8_t init2[] = {INIT_SEQ1, INIT_SEQ2};
    i2c_master_start();
    i2c_master_write(init2, 2, LCD_ADDR);
    i2c_master_stop();

    systick_delay(1);

    uint8_t init3[] = {INIT_SEQ1, INIT_SEQ2};
    i2c_master_start();
    i2c_master_write(init3, 2, LCD_ADDR);
    i2c_master_stop();

    systick_delay(1);

    uint8_t init4[] = {SET_4BIT_MODE1, SET_4BIT_MODE2};
    i2c_master_start();
    i2c_master_write(init4, 2, LCD_ADDR);
    i2c_master_stop();

    systick_delay(1);

    lcd_send_cmd(LCD_FUNCSET);
    lcd_send_cmd(LCD_DISPOFF);
    lcd_send_cmd(LCD_CLEAR);

    systick_delay(2000);
    
    lcd_send_cmd(LCD_ENTRYMODE);
    lcd_send_cmd(LCD_DISPON);


    return;
}


// print something to the lcd
/*
 * @param (input) null terminated string
 */
void lcd_print(char *input){
    (void) input;
    
    uint32_t idx = 0;
    while(input[idx] != '\0') {
        uint8_t character[] = {(input[idx] & MSB4_MASK) | CHAR_E_HIGH,
                       (input[idx] & MSB4_MASK) | CHAR_E_LOW,
                       ((input[idx] & LSB4_MASK) << 4) | CHAR_E_HIGH,
                       ((input[idx] & LSB4_MASK) << 4) | CHAR_E_LOW};
        i2c_master_start();
        i2c_master_write(character, 4, LCD_ADDR);
        i2c_master_stop();
        idx += 1;
    }
    return;
}

// @param row - 0 for first line, 1 for second line
// @param column - 0 to F from left to right
void lcd_set_cursor(uint8_t row, uint8_t col){
    uint8_t cursor_code[] = {LCD_CURSOR | (row << 6) | E_HIGH,
                 LCD_CURSOR | (row << 6) | E_LOW,
                 (col << 4) | E_HIGH,
                 (col << 4) | E_LOW};
    i2c_master_start();
    i2c_master_write(cursor_code, 4, LCD_ADDR);
    i2c_master_stop();
    return;
}

// clear the lcd
void lcd_clear() {
    lcd_send_cmd(LCD_CLEAR);

    systick_delay(2000);
    return;
}
