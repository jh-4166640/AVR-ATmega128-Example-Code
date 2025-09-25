/*
 * SPI_TEST.c
 *
 * Created: 2025-09-23 오전 11:44:49
 * Author : Jiheon Choi
 */ 

#define F_CPU 14745600UL   // Default CPU speed : 14.7456 MHz (override in project if needed)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "spi_gcc.h"
#include "lcd_gcc.h"

#define MAXLEN 128
#define CR	   0x0D				// Carriage Return hex value
#define STR_SIGN  0xEE			// String 전송 시작한다고 알려주기 위한 값
#define CHATTERING_DELAY 150	// 프로테우스에서 CPU 속도 마다 다름

uint8_t key = 0 ,key_flag = 0;	// button 입력
uint8_t send_data =0;			// 전송할 데이터
uint8_t send_data_head = 0 ,send_data_last = 0; // 전송할 문자의 Range
volatile const uint8_t data_str[] = "SPI connected!";			// polling 방식으로 보낼 문자열
volatile const uint8_t data_ISR_str[] = "SPI STC ISR Tx";		// Interrupt 방식으로 보낼 문자열
uint8_t ptr_idx=0;								// Interrupt 방식으로 보낼 문자열의 인덱스 지정용
uint8_t clear_send_str =1;						// 문자열 전송 다 했는지 확인 0이면 전송 중 
uint8_t idx = 13, row =0;						// LCD pos 핸들링용임 

void timer1_ctc_1hz_init(void){
	// 14.7456MHz / 1024 = 14400, 1 Hz -> OCR1A = 14399
	TCCR1B = 0;               // stop
	TCCR1A = 0;
	OCR1A  = 1439; // modify // 0.1초
	TCCR1B = (1<<WGM12) | (1<<CS12) | (1<<CS10);  // CTC, prescale 1024
	TIMSK |= (1<<OCIE1A);     // Compare A interrupt
}
ISR(TIMER1_COMPA_vect){	// 0.1초마다 polling 방식으로 전송
	if(key_flag & 0x03) // A만 보내거나, A~Z까지 한문자씩 보내는 용도로 쓰기 위함
	{
		SPI_Master_Send(send_data);
		LCD_Pos(0,13);
		LCD_Char(send_data);
		if(send_data < send_data_last) send_data++;	
		else send_data = send_data_head;
	}
}
ISR(SPI_STC_vect)	// 인터럽트로 문자열 전송하기 위함
{				
	if(clear_send_str == 0){	// 전송 중
		if(*(data_ISR_str+ptr_idx)!='\0'){	
			if(SPDR!=STR_SIGN)	// 보내려는 값이 문자열 데이터를 송신하겠다는 헤더문자가 아니면
			{
				LCD_Pos(row,idx++);
				LCD_Char(*(data_ISR_str+ptr_idx));
				if(idx>=16){
					idx = 0;
					row = 1;
				}	
			}
			SPDR=*(data_ISR_str+ptr_idx++);
		} else { // 마지막 문자면
			SPDR = CR;
			clear_send_str = 1;
		}
	}
	else {
		SPI_CS_HIGH();	// slave 선택 취소
		SPCR &= (~(1<<SPE) & ~(1<<SPIE));	// SPI 통신종료와 SPI 인터럽트 불허가
		TIMSK |= (1<<OCIE1A);				// 타이머 인터럽트는 다시 활성화
	}
}

static inline void LCD_init_print(void)
{
	LCD_Clear();
	LCD_Pos(0,0); 
	LCD_Str("Master Send:");
}

int main(void)
{
    DDRC = 0x00;
	PORTC = 0xFF;
	
	Init_SPI_Master();
	timer1_ctc_1hz_init();
	sei();
	LCD_Init();
	LCD_init_print();
	
	while(1){
		key = (PINC&0xff);
		switch(key)
		{
			case 0xF7:
				_delay_ms(CHATTERING_DELAY); // 채터링 제거용
				SPCR |= (1<<SPE);	// SPI 통신 허가
				if(key_flag & 0x04) LCD_init_print();
				idx = 13;
				row = 0;
				cli();
				TIMSK &= ~(1<<OCIE1A); // 타이머 비교일치 인터럽트 불허
				key_flag = 0x04;
				SPCR |= (1<<SPIE);	// SPI 인터럽트 허가
				ptr_idx=0;
				clear_send_str=0;
				sei();
				SPI_CS_LOW();	// slave 선택 On
				SPDR = STR_SIGN; // 쓰레기 값으로 인터럽트 호출
				
				break;
			case 0xFB:
				_delay_ms(CHATTERING_DELAY); // 채터링 제거용
				SPCR |= (1<<SPE); // SPI 통신 허가
				if(key_flag & 0x04) LCD_init_print();
				key_flag = 0x04;
				cli();
				idx = 13;
				row = 0;
				SPI_Master_Send(STR_SIGN); // 문자열 전송을 시작하겠다는 정보를 가진 헤더 문자 전송
				_delay_ms(50);				// Slave 준비 대기
				for (uint8_t i = 0;data_str[i];i++)
				{
					if(idx >= 16)
					{
						idx = 0;
						row = 1;
					}
					uint8_t temp_buffer = data_str[i];
					LCD_Pos(row,idx++);
					SPI_Master_Send(temp_buffer);
					LCD_Char(temp_buffer);
					_delay_ms(50);
				}
				SPI_Master_Send(CR);	// 전송 완료 후 문자열 전송 완료됨 보내기
				sei();
				SPCR &= ~(1<<SPE); // SPI 통신 허가
				break;
			
			case 0xFD:
				_delay_ms(CHATTERING_DELAY); // 채터링 제거용
				SPCR |= (1<<SPE); // SPI 통신 허가
				if(key_flag & 0x04) LCD_init_print();
				cli();
				send_data_head = 'A';	// A부터
				send_data_last = 'Z';	// Z까지
				send_data = send_data_head;
				key_flag = 0x02;
				sei();
				_delay_ms(10);
				break;
			
			case 0xFE:
				_delay_ms(CHATTERING_DELAY); // 채터링 제거용
				SPCR |= (1<<SPE); // SPI 통신 허가
				if(key_flag & 0x04) LCD_init_print();
				cli();
				send_data_head = 'A';
				send_data_last = 'A';
				send_data = send_data_head;
				key_flag = 0x01;
				sei();
				_delay_ms(10);
				break;
				
			case 0x7F:
			case 0xBF:
			case 0xDF:
			case 0xEF:
				/*	설정하지 않은 버튼들이 눌렸을 때	*/
				_delay_ms(CHATTERING_DELAY); // 채터링 제거용 
				SPCR &= ~(1<<SPE); // SPI 통신 불허
				if(key_flag & 0x04) LCD_init_print();
				key_flag = 0;
				LCD_Clear();
				LCD_Pos(0,0); LCD_Str("Master Send:");
				_delay_ms(10);
				break;
			default:
				break;
		}
		_delay_ms(10);
	}
}

