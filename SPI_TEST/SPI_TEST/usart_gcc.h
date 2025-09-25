#ifndef _INCLUDE_UART_H__
#define _INCLUDE_UART_H__

// AVR-GCC / Microchip Studio compatible USART1 helpers (ATmega128)
// - Fixed include guards
// - Use standard register names and bit ops
// - Keeps original API

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define RX_Int   0   // enable RX interrupt
#define TX_Int   1   // enable UDRE interrupt
#define RXTX_Int 2   // enable both

// baud = 0 : 9600 bps, baud = 1 : 115200 bps  (for F_CPU = 14.7456 MHz)
static inline void Init_USART1(uint8_t baud);
static inline void Init_USART1_IntCon(uint8_t baud, uint8_t Int_type);
static inline void putch_USART1(char data);
static inline void puts_USART1(char *str);
static inline char getch_USART1(void);

static inline void Init_USART1(uint8_t baud)
{
    UCSR1A = 0x00;                         // normal speed
    UCSR1B = _BV(RXEN1) | _BV(TXEN1);      // enable RX, TX
    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10);    // 8N1

    switch (baud) {
        case 0: // 9600 @ 14.7456 MHz
            UBRR1H = 0x00;
            UBRR1L = 95;
            break;
        case 1: // 115200 @ 14.7456 MHz
            UBRR1H = 0x00;
            UBRR1L = 0x07;
            break;
        default:
            break;
    }
}

static inline void Init_USART1_IntCon(uint8_t baud, uint8_t Int_type)
{
    UCSR1A = 0x00; // normal speed

    switch (baud) {
        case 0: // 9600
            UBRR1H = 0x00;
            UBRR1L = 95;
            break;
        case 1: // 115200
            UBRR1H = 0x00;
            UBRR1L = 0x07;
            break;
        default:
            break;
    }

    // Enable RX/TX and requested interrupts
    switch (Int_type) {
        case RX_Int:
            UCSR1B = _BV(RXCIE1) | _BV(RXEN1) | _BV(TXEN1);
            break;
        case TX_Int:
            UCSR1B = _BV(UDRIE1) | _BV(RXEN1) | _BV(TXEN1);
            break;
        case RXTX_Int:
            UCSR1B = _BV(RXCIE1) | _BV(UDRIE1) | _BV(RXEN1) | _BV(TXEN1);
            break;
        default:
            UCSR1B = _BV(RXEN1) | _BV(TXEN1);
            break;
    }

    UCSR1C = _BV(UCSZ11) | _BV(UCSZ10); // 8N1
}

static inline void putch_USART1(char data)
{
    while (!(UCSR1A & _BV(UDRE1))) {;}
    UDR1 = data;
}

static inline void puts_USART1(char *str)
{
    while (*str != 0) {
        putch_USART1(*str++);
    }
}

static inline char getch_USART1(void)
{
    while (!(UCSR1A & _BV(RXC1))) {;}
    return UDR1;
}

#endif // _INCLUDE_UART_H__
