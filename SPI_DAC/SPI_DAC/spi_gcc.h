#ifndef _INCLUDE_SPI_H__
#define _INCLUDE_SPI_H__

// AVR-GCC / Microchip Studio compatible SPI helpers (ATmega128)
// - Replaced CodeVision style PORTB.0 with standard bit ops (_BV, PBx)
// - Use cli()/sei() instead of writing SREG
// - Keeps original API names for drop-in use
// - Header-only via static inline

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

// Chip Select (SS) lines
#define SPI_CS_PORT   PORTB
#define SPI_CS_DDR    DDRB
#define SPI_CS_PIN    PB0       // PORTB.0

#define SPI_CS2_PORT  PORTE
#define SPI_CS2_DDR   DDRE
#define SPI_CS2_PIN   PE6       // PORTE.6

#define SPI_CS_HIGH()   (SPI_CS_PORT  |= _BV(SPI_CS_PIN))
#define SPI_CS_LOW()    (SPI_CS_PORT  &= ~_BV(SPI_CS_PIN))
#define SPI_CS2_HIGH()  (SPI_CS2_PORT |= _BV(SPI_CS2_PIN))
#define SPI_CS2_LOW()   (SPI_CS2_PORT &= ~_BV(SPI_CS2_PIN))

// SPI pins on ATmega128 live on PORTB
#define SS    PB0
#define SCK   PB1
#define MOSI  PB2
#define MISO  PB3

static volatile char *PtrToStrChar;  		// pointer to current char in string (used by interrupt-driven TX)
static volatile char  ClearToSend = 1;

static inline void Init_SPI_Master(void);
static inline void Init_SPI_Master_IntContr(void);
static inline void Init_SPI_Slave_IntContr(void);
static inline uint8_t SPI_Master_Send(uint8_t data);
static inline uint8_t SPI_Master_Receive(void);
static inline uint8_t SPI_Slave_Receive(void);
static inline void SPI_Master_Send_IntContr(unsigned char *TextString);

// Master init (polling mode)
static inline void Init_SPI_Master(void)
{
    // MOSI, SS, SCK output; MISO input
    DDRB |= _BV(MOSI) | _BV(SS) | _BV(SCK);
    DDRB &= ~_BV(MISO);

    // Enable SPI, Master, Mode 1 (CPHA=1, CPOL=0), fosc/128
    //SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPHA) | (3 << SPR0);
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << CPHA); //fosc/4
    SPSR = 0x00;

    // Ensure CS is output high (inactive)
    SPI_CS_DDR |= _BV(SPI_CS_PIN);	// DDR을 출력 포트로 
    SPI_CS_HIGH();					// SS 신호선을 대기상태(high)로 설정 복귀
}

// Master init (interrupt mode)
static inline void Init_SPI_Master_IntContr(void)
{
    cli();
	 // I/O를 master로 동작으로 설정, (MOSI, SS, SCK 출력, MISO 입력)
    DDRB |= _BV(MOSI) | _BV(SS) | _BV(SCK);
    DDRB &= ~_BV(MISO);

    // Enable SPI + SPI interrupt, Master, Mode 1, fosc/128
    SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<CPHA) | (3<<SPR0);
    SPSR = 0x00;

    // CS pin as output, high
    SPI_CS_DDR |= _BV(SPI_CS_PIN);
    SPI_CS_HIGH();

    sei();
}

// Slave init (interrupt mode)
static inline void Init_SPI_Slave_IntContr(void)
{
    // MISO output in slave
    DDRB |= (1 << MISO);

    // Enable SPI + SPI interrupt, Mode 1
    SPCR = (1<<SPIE) | (1<<SPE) | (1<<CPHA) | (3<<SPR0);
	// SPCR을 슬레이브 INT 동작으로 초기화, 클럭 : SCK = fosc/128
    SPSR = 0x00;
    sei();
}

// Send one byte as master (polling); returns received byte
static inline uint8_t SPI_Master_Send(uint8_t data)
{
    SPI_CS_LOW();                // assert CS (optional if handled outside)
    // _delay_us(10);            // if slave needs settling time

    SPDR = data;
    while (!(SPSR & _BV(SPIF))) {;}
    uint8_t rx = SPDR;           // read to clear SPIF

    SPI_CS_HIGH();               // deassert CS
    return rx;
}

static inline uint8_t SPI_Master_Receive(void)
{
    // To receive, send dummy 0xFF
    SPDR = 0xFF;
    while (!(SPSR & _BV(SPIF))) {;}
    return SPDR;
}

static inline uint8_t SPI_Slave_Receive(void)
{
    while (!(SPSR & _BV(SPIF))) {;}
    return SPDR;
}

// Kick off interrupt-driven TX of a zero-terminated string
static inline void SPI_Master_Send_IntContr(unsigned char *TextString)
{
    if (ClearToSend == 1) {
        PtrToStrChar = (volatile char*)TextString;
        SPDR = *PtrToStrChar;  // first char
        ClearToSend = 0;
    }
}

#endif // _INCLUDE_SPI_H__