/*
 * SPI_TEST_slave.c
 *
 * Created: 2025-09-23 오전 11:50:56
 * Author : Jiheon Choi
 */ 
#define F_CPU 14745600UL   // Default CPU speed : 14.7456 MHz (override in project if needed)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "spi_gcc.h"
#include "lcd_gcc.h"

#define CR		0x0D
#define STR_SIGN  0xEE
static volatile uint8_t SPI_REC_Char = 0;

/* ================= RX Ring Buffer ================= */
#define Q_SIZE 128u
#define Q_MASK (Q_SIZE-1u)
static volatile uint8_t qbuf[Q_SIZE];
static volatile uint8_t qhead = 0; // next to pop
static volatile uint8_t qtail = 0; // next to push
static volatile uint8_t q_highwater = 0; // for stats

static inline int enQue(uint8_t b){
	uint8_t next = (uint8_t)((qtail + 1u) & Q_MASK);
	if(next == qhead) return -1; // full
	
	qbuf[qtail] = b;
	qtail = next;
	uint8_t size = (uint8_t)((qtail - qhead) & Q_MASK);
	if(size > q_highwater) q_highwater = size;
	return (int)size;
}
static inline int deQue(uint8_t* out){
	if(qhead == qtail) return -1; // empty
	*out = qbuf[qhead];
	qhead = (uint8_t)((qhead + 1u) & Q_MASK);
	return (int)((qtail - qhead) & Q_MASK);
}

ISR(SPI_STC_vect){
	SPI_REC_Char = SPDR;
	enQue(SPI_REC_Char); // 수신 된 문자 Ring buffer에 저장
}

int main(void)
{
	uint8_t lcd_row=0, lcd_col=12;	// LCD Pos 제어용 
	uint8_t Rx_Str_mode = 0;		// 문자열 수신 모드 확인용 변수
	uint8_t Rx_data = 0;			// Ring buffer에서 꺼내서 저장할 변수
	int Rx_res = 0;					// deque 결과 저장용
    Init_SPI_Slave_IntContr();
	LCD_Init();
	LCD_Pos(0,0); LCD_Str("Slave Rec: ");
	sei();
    while (1) 
    {
		Rx_res=deQue(&Rx_data);
		_delay_ms(10);
		if(Rx_res>-1)			// ring buffer에 데이터가 있다면
		{
			if(Rx_data == STR_SIGN)		// 수신된 문자가 문자열 전송 시작 문자일 경우
			{
				Rx_Str_mode = 1;		// 문자열 수신 모드로 전환 
				Rx_data=' ';			// 제어용 문자 데이터 비우기
				lcd_col=11;				
				lcd_row=0;
			} else if(Rx_data == CR) { // 수신된 문자가 문자열 전송 종료 문자일 경우
				Rx_Str_mode = 0;		// 문자열 수신 모드 해제
				lcd_col=12;	
				lcd_row=0;
				_delay_ms(100);
				LCD_Clear();
				LCD_Pos(0,0); LCD_Str("Slave Rec: ");
				Rx_data=' ';			// 제어용 문자 제거
			}
			if(Rx_Str_mode){			// 문자열 수신 모드
				lcd_col++;
				if(lcd_col >=16)
				{
					lcd_col=0;
					lcd_row=1;
				}
			} else {					// 문자 수신 모드
				lcd_col=12;
				lcd_row=0;
			}
			LCD_Pos(lcd_row, lcd_col);
			LCD_Char(Rx_data);
			_delay_ms(1);	
		}
    }
}

