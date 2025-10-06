/*
 * SPI_MCP23S08.c
 *
 * Created: 2025-10-06 오후 5:12:57
 * Author : 최지헌
 */ 

#define F_CPU	14745600UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "lcd_gcc.h"
#include "spi_gcc.h"
#include "MCP23S08.h"
uint8_t on_led = 0;

// INT0: falling edge (MCP23S08 INT: active-low). INT0=PD0 on ATmega128.
static inline void INT0_Init(void){
	EIMSK = (1<<INT0);
	EICRA = (1<<ISC01);			// Falling edge
}


// -------- 이벤트 전달 --------
ISR(INT0_vect){
	g_cap = MCP_Read(INTCAP);	// 변화 당시 값 캡쳐 + INT 클리어
	g_evt = 1;
}

static inline char hex1(uint8_t n){ n &= 0x0F; return (n<10)?('0'+n):('A'+n-10); }
	
void LED_PRINT(uint8_t led_mode)
{
	if(led_mode == 0x01)
	{
		MCP_Write(GPIO,on_led);
		_delay_ms(100);
		on_led = ((on_led << 1) | 1) & 0x0F;
		if(on_led == 0x0F) on_led = 0x0E;
	}
	else if(led_mode == 0x02)
	{
		MCP_Write(GPIO,on_led);
		_delay_ms(100);
		on_led = ((on_led >> 1) | 0x08) & 0x0F;
		if(on_led == 0x0F) on_led = 0x07;
	}
	else if(led_mode == 0x04 || led_mode == 0x08)
	{
		MCP_Write(GPIO,on_led);
		_delay_ms(100);
		on_led = ~(on_led)&0x0F;
	}
}


int main(void)
{
	/* Replace with your application code */
	
	uint8_t now=0, in_nib=0, led= 0;
	uint8_t led_mode = 0;
	
	MCP_Init_anychange();
	
	#ifdef _MCP23S08_Event_Mode_
	INT0_Init();
	#else
	g_evt = 1;
	#endif
	
	sei(); //Global Interrupt Set
	
	while (1)
	{
		if(g_evt)
		{
			_delay_ms(5);
			
			#ifndef _MCP23S08_Event_Mode_
			//MCP Nomal function
			now= MCP_Read(GPIO);
			in_nib = (now>>4)&0x0F;		//상위 4비트
			#else
			//MCP Event(interrupt) function
			in_nib = (g_cap>>4)&0x0F;	//상위 4비트
			g_evt=0;	//INT0 Flag reset...
			#endif
			
			led = (~in_nib)&0x0F; //눌림=0 -> LED ON(active-low 가정)
			// 눌렸을 때만 led_mode를 설정하도록 함
			if(led == 0x01)			{on_led = 0x0E; led_mode=led;} // 왼쪽 shift
			else if(led == 0x02)	{on_led = 0x07; led_mode=led;} // 오른쪽 shift
			else if(led == 0x04)	{on_led = 0x0F; led_mode=led;} // 전체 toggle 
			else if(led == 0x08)	{on_led = 0x03; led_mode=led;} // 2개씩 교대 점멸
			
			if(led_mode != 0x00) LED_PRINT(led_mode);
			

			// LCD 업데이트
			LCD_Pos(0,8); LCD_Char('0'); LCD_Char(hex1(in_nib));
			LCD_Pos(1,8); LCD_Char('0'); LCD_Char(hex1(led));
		}
		else;
	}
}
