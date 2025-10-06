/*
 * SPI_DAC.c
 * Created: 2025-09-30 오전 10:43:24
 * Author : 최지헌
 */ 

#define F_CPU	14745600UL	
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "spi_gcc.h"
#include <math.h>


#define SAWTOOTH_WAVE	1
#define SQUARE_WAVE		2
#define SIN_WAVE		3
#define MAX_STEP		100
#define DAC_MAX			4095

uint8_t sel_wave = 0;
uint16_t DAC_data =0;
uint16_t COMP_cnt = 0;
uint16_t sin_table[MAX_STEP]; 

static void MCP4921_SPI_Write(uint16_t val12);
static void MCP4921_Init(void);
static void sin_table_Init(void)
{
	double step = 2.0 * M_PI / 100.0;
	double angle = 0;
	for(uint8_t i = 0;i<MAX_STEP;i++)
	{
		// sin() : -1~1 이므로 단극성 0~2로 올리기
		sin_table[i] = (uint16_t)((sin(angle) + 1.0) * (DAC_MAX/2.0)); // 0 -> 0, 1 -> 4095/2, 2 -> 4095
		angle+=step;
	}
}
static void Button_Init(void)
{
	DDRD = 0x00;
	EIMSK = (1<<INT0) | (1<<INT1) | (1<<INT2);
	EICRA = (1<<ISC01) | (1<<ISC11) | (1<<ISC21);
}
void timer0_ctc_init() {
	// Timer0 초기화
	TCCR0 = (1<<WGM01) | (1<<CS02);    // CTC 모드
	OCR0 = (F_CPU / (64UL * MAX_STEP * 100)) - 1; // 100Hz 기준
	TCNT0 = 0;              // 카운터 초기화
	TIMSK |= (1<<OCIE0);   // Compare Match A 인터럽트 enable
}

ISR(INT0_vect)
{
	sel_wave = SAWTOOTH_WAVE;
	TCNT0 = 0;
	DAC_data=0;
}
ISR(INT1_vect)
{
	sel_wave = SQUARE_WAVE;
	TCNT0 = 0;
	DAC_data=0;
	COMP_cnt =0;
}
ISR(INT2_vect)
{
	sel_wave = SIN_WAVE;
	TCNT0 = 0;
	DAC_data = 0;
	COMP_cnt =0;
}
ISR(TIMER0_COMP_vect)
{
	if(sel_wave == SAWTOOTH_WAVE)
	{
		DAC_data += (DAC_MAX/MAX_STEP) + 1;
		MCP4921_SPI_Write(DAC_data);
		if(DAC_data >= DAC_MAX) DAC_data =0;
	}	
	else if(sel_wave == SQUARE_WAVE)
	{
		MCP4921_SPI_Write(DAC_data);
		COMP_cnt++;
		if(COMP_cnt >= 5) // 타이머 비교일치 100번이면 10ms
		{				  // 10ms / 10 -> 1ms, duty 50% -> 0.5ms 마다 toggle
			DAC_data ^= DAC_MAX;
			COMP_cnt = 0;
		}
	}
	else if(sel_wave == SIN_WAVE)
	{
		MCP4921_SPI_Write(sin_table[COMP_cnt++]);
		if(COMP_cnt >= 100) COMP_cnt = 0;
	}
	
}

int main(void)
{
    
	DDRE = 0xff;
	sin_table_Init();
	MCP4921_Init();
	Button_Init();
	timer0_ctc_init();
	sei();
    while (1) 
    {
		
    }
}

static void MCP4921_SPI_Write(uint16_t val12)
{
	//MCP4921 상위 byte 0b0011xxxx
	uint8_t msb = (uint8_t)(0b00110000 | ((val12 >> 8) & 0x0F));	//16비트인 val12를 오른쪽으로 8비트 시키면 하위 8비트 사라지고 상위 8비트가 하위로 감
	uint8_t lsb = (uint8_t)(val12 & 0xFF);						//8비트로 잘라버리면 상위 8비트 날라감
	
	//SPI_CS2_DDR |= (1<<SPI_CS2_PIN);
	SPI_CS2_LOW();
	SPI_Master_Send(msb);
	SPI_Master_Send(lsb);
	SPI_CS2_HIGH();
	
}
static void MCP4921_Init(void)
{
	Init_SPI_Master();
	MCP4921_SPI_Write(0);
	//_delay_ms(1);
}

