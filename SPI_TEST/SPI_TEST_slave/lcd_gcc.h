#ifndef _INCLUDE_LCD_H__
#define _INCLUDE_LCD_H__

// AVR-GCC / Microchip Studio compatible LCD driver (ATmega128)
// - Fixed include guards
// - Keeps original API
// - Header-only via static inline to avoid multiple definition errors

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#ifndef F_CPU
#define F_CPU 14745600UL   // Default CPU speed : 14.7456 MHz (override in project if needed)
#endif

// LCD data bus (8-bit)
#define LCD_WDATA   PORTA        // write data
#define LCD_WINST   PORTA        // write command
#define LCD_RDATA   PINA         // read data

// LCD control signals live on PORTG bits 0..2
#define LCD_CTRL    PORTG
#define LCD_CTRL_DDR DDRG

#define LCD_EN  0                // Enable bit position
#define LCD_RW  1                // Read(1)/Write(0)
#define LCD_RS  2                // Data(1)/Command(0)

#define RIGHT 1
#define LEFT  0

static inline void LCD_Data(uint8_t ch);
static inline void LCD_Comm(uint8_t command);
static inline void LCD_Delay(uint8_t ms);
static inline void LCD_Char(uint8_t ch);
static inline void LCD_Str(const char *str);
static inline void LCD_Pos(uint8_t x, uint8_t y);
static inline void LCD_Clear(void);
static inline void LCD_PORT_Init(void);
static inline void LCD_Init(void);
static inline void LCD_Display_Shift(uint8_t p);
static inline void LCD_Cursor_Shift(uint8_t p);
static inline void LCD_Cursor_Home(void);

static inline void LCD_Data(uint8_t ch)
{
    LCD_CTRL |= (1 << LCD_RS);    // RS=1 -> data
    LCD_CTRL &= ~(1 << LCD_RW);   // RW=0 -> write
    LCD_CTRL |= (1 << LCD_EN);    // E high
    _delay_us(50);
    LCD_WDATA = ch;               // put data
    _delay_us(50);
    LCD_CTRL &= ~(1 << LCD_EN);   // E low
}

static inline void LCD_Comm(uint8_t command)
{
    LCD_CTRL &= ~(1 << LCD_RS);   // RS=0 -> command
    LCD_CTRL &= ~(1 << LCD_RW);   // RW=0 -> write
    LCD_CTRL |= (1 << LCD_EN);    // E high
    _delay_us(50);
    LCD_WINST = command;          // put command
    _delay_us(50);
    LCD_CTRL &= ~(1 << LCD_EN);   // E low
}

static inline void LCD_Delay(uint8_t ms)
{
    while (ms--) _delay_ms(1);
}

static inline void LCD_Char(uint8_t ch)
{
    LCD_Delay(1);
    LCD_Data(ch);
}

static inline void LCD_Str(const char *str)
{
    while (*str != 0) {
        LCD_Char((uint8_t)*str++);
    }
}

static inline void LCD_Pos(uint8_t x, uint8_t y)
{
    // x: row (0..1), y: column (0..15)
    LCD_Comm(0x80 | (x*0x40 + y));
}

static inline void LCD_Clear(void)
{
    LCD_Comm(0x01);
    LCD_Delay(2);
}

static inline void LCD_PORT_Init(void)
{
    DDRA  = 0xFF;         // PORTA as output (data bus)
    LCD_CTRL_DDR |= 0x07; // PORTG bits 0..2 as output (E,RW,RS)
}

static inline void LCD_Init(void)
{
    LCD_PORT_Init();

    LCD_Comm(0x38);   // 8-bit, 2-line
    LCD_Delay(4);
    LCD_Comm(0x38);
    LCD_Delay(4);
    LCD_Comm(0x38);
    LCD_Delay(4);
    LCD_Comm(0x0E);   // display on, cursor on
    LCD_Delay(2);
    LCD_Comm(0x06);   // entry mode: increment
    LCD_Delay(2);
    LCD_Clear();
}

static inline void LCD_Display_Shift(uint8_t p)
{
    _delay_ms(10);
    if (p == RIGHT) {
        LCD_Comm(0x1C);
        LCD_Delay(1);
    } else if (p == LEFT) {
        LCD_Comm(0x18);
        LCD_Delay(1);
    }
}

static inline void LCD_Cursor_Shift(uint8_t p)
{
    if (p == RIGHT) {
        LCD_Comm(0x14);
        LCD_Delay(1);
    } else if (p == LEFT) {
        LCD_Comm(0x10);
        LCD_Delay(1);
    }
}

static inline void LCD_Cursor_Home(void)
{
    LCD_Comm(0x02);
    LCD_Delay(2);
}

#endif // _INCLUDE_LCD_H__
