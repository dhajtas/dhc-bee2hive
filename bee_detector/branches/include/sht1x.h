#ifndef	SHT1x_H
#define SHT1x_H

//#define		SHTSCKA		( PORTF & 0x08 )
//#define		SHTSCKB		( PORTB & 0x80 )
//#define		SHTSCKC		( PORTA & 0x08 )
//#define		SHTSCKD		( PORTA & 0x80 )
//#define		SHTODTA		( DDRF & 0x01 )
//#define		PINADC		PINF
//#define		PORTDIO		PORTA
//#define		DDRDIO		DDRA
//#define		PINDIO		PINA

#define		SHT_PORT		PORT(I2C_P)
#define		SDA_bm			SDA0|SDA1|SDA2|SDA3|SDA4|SDA5
#define		SCL_bm			SCL0|SCL1

#define		SHT_MEAS_TEMP	0x03
#define		SHT_MEAS_HUM	0x05
#define 	SHT_READ_STAT	0x07
#define		SHT_WRITE_STAT	0x06
#define		SHT_RESET		0x1E

//-----------------------------------------------------------------------------------------------//
//		structure definitions 
//-----------------------------------------------------------------------------------------------//

typedef struct SHT
{
	uint16_t TEMP;
	uint16_t HUM;
	uint8_t STATUS;
} SHT;

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//extern uint8_t SHTmask;		mask.SHTM instead
extern SHT SHTbuff[4];


//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

void SHT_meas(uint8_t);

void SHT_meas_dummy(void);

void SHT_read_stat(void);

void SHT_write_stat(uint8_t);

void SHT_reset(void);		//bolo uint8_t

void SHT_connection_reset(uint8_t);

uint8_t SHT_send8_t(uint8_t,uint8_t);

uint8_t SHT_rcv8_t(uint8_t, uint8_t);

void SHT_TRX_start(uint8_t);

void SHT_send_bit(uint8_t, uint8_t);

uint8_t SHT_rcv_bit(uint8_t);

//void SHT_send_ack(uint8_t, uint8_t);

//uint8_t SHT_rcv_ack(uint8_t);

void SHT_wait(uint8_t);

void SHT_init(void);

void SHT_clk(uint8_t);

void SHT_sw_data_dir(uint8_t, uint8_t);


// only SHT # 0, 2, 3 will be serviced for DS3 device (0 - internal SHT port)

#endif //SHT1x_H
