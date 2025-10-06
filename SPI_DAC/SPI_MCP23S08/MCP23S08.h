/*
 * MCP23S08.h
 *
 * Created: 2025-10-06 오후 5:19:43
 *  Author: 최지헌
 */ 

#ifndef MCP23S08_H_
#define MCP23S08_H_
// ---- MCP23S08 정의 (A2..A0=000)----
#define MCP23S08_ADDR_W  0x40
#define MCP23S08_ADDR_R  0x41

// MCP 동작 모드 설정을 위한 함수 활성화 코드
// 주석처리 : 일반 동작
// 코드활성 : 인터럽트(이벤트) 동작
//#define _MCP23S08_Event_Mode_

enum MCP23S08_REG_ADDRESS{
	IODIR	= 0x00,
	IPOL	= 0x01,
	GPINTEN	= 0x02,
	DEFVAL	= 0x03,
	INTCON	= 0x04,
	IOCON	= 0x05,
	GPPU	= 0x06,
	INTF	= 0x07,
	INTCAP	= 0x08,
	GPIO	= 0x09,
	OLAT	= 0x0A,
};

volatile uint8_t g_evt=0;
volatile uint8_t g_cap=0;

// ---- MCP23S08 R/W ----
static inline void spi_Xfer(uint8_t data)
{
	SPDR = data;
	while(!(SPSR&(1<<SPIF)));
}
static inline uint8_t spi_Xfer_return(void)
{
	spi_Xfer(0x00); //dummy data Xfer
	
	return SPDR;
}
static inline void MCP_Write(uint8_t reg, uint8_t val){
	SPI_CS_LOW();
	
	spi_Xfer(MCP23S08_ADDR_W);
	spi_Xfer(reg);
	spi_Xfer(val);
	
	SPI_CS_HIGH();
}
static inline uint8_t MCP_Read(uint8_t reg){
	uint8_t SPI_Rx=0;
	
	SPI_CS_LOW();
	
	spi_Xfer(MCP23S08_ADDR_R);
	spi_Xfer(reg);
	SPI_Rx=spi_Xfer_return();
	
	SPI_CS_HIGH();
	
	return SPI_Rx;
}

static inline void MCP23S08_SPI_Init(void)
{
	// MOSI, SS, SCK output; MISO input
	SPI_CS_DDR |= _BV(MOSI) | _BV(SS) | _BV(SCK);
	SPI_CS_DDR &= ~_BV(MISO);
	
	SPCR = (1<<SPE)|(1<<MSTR)|(0<<CPOL)|(0<<CPHA)|(3<<SPR0);
	SPSR = 0x00;

	// Ensure CS is output high (inactive)
	SPI_CS_HIGH();		// SS 신호선을 대기상태(high)로 설정 복귀
}

// -------- 초기화 (any-change 인터럽트) --------
static inline void MCP_Init_anychange(void){
	
	MCP23S08_SPI_Init();
	
	LCD_Init(); LCD_Clear();
	LCD_Pos(0,0); LCD_Str("IN  = 0x--");
	LCD_Pos(1,0); LCD_Str("OUT = 0x--");
	
	MCP_Write(IODIR, 0xF0);   // 7..4 in, 3..0 out
	MCP_Write(IPOL , 0x00);   // 정논리
	
	//MCP Interrupt Setting..
	//mcp_write(GPINTEN, 0x00); // MCP Interrupt All disable.
	MCP_Write(GPINTEN, 0xF0);	// 상위 4비트 인터럽트 기능 활성화 (MCP Interrupt)
	MCP_Write(DEFVAL, 0xF0);	// 상위 4비트 변화 감지 레벨 설정, 'H' 이기 떄문에 GPIO가 'L'가 되면 인터럽트 발생
	MCP_Write(INTCON , 0xF0);	// Set '1' = interrupt-on-change, Set / '0' = It is compared against the previous pin value
	// 즉, 1로 설정하면 인터럽트 발생 조건에 따라 인터럽트 발생, 0으로 설정하면 PIN 논리 상태변화에 따라 매번 인터럽트 발생

	// IOCON: HAEN(하드웨어 주소 enable)=1, SEQOP=1(연속 주소 자동증가 비활성 — 가독성 목적)
	// MCP_Write(IOCON, (1<<5));	// Sequential Operation mode bit : Sequential operation disabled, address pointer does not increment.
	// MCP_Write(IOCON, (1<<3) | (1<<5));
	
	//MCP_Write(GPPU , 0xF0);   // 상위 4비트 풀업 -> 회로에 H/W 구성을 하였습니다.
	//MCP_Write(INTF , 0x00);	// GPINTEN 기능 설정으로 활성화된 인터럽트 플래그(해당 레지스터는 읽기 전용 입니다.)

	MCP_Write(GPIO,0x0F);	// 하위 4bit All 'H' -> All LED Off...
	MCP_Read(INTCAP);		// 인터럽트 초기화
}


#endif /* MCP23S08_H_ */