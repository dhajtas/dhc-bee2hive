#include <avr/io.h>
#include <inttypes.h>

//#include "1wire.h"
#include "hw.h"
#include "sht1x.h"


//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//uint8_t SHTmask;		mask.SHTM instead
SHT SHTbuff[4];

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

void SHT_meas(uint8_t command)
{
	uint16_t msk=0x0001;
	uint8_t crc, y;
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
//		SHT_connection_reset(y);
		SHT_TRX_start(y);
		SHT_send8_t(command,y);
//		SHT_wait(y);
		}
		msk = msk<<4;
	}
	
	if(command == 0x03)
		DELAY_MS(220);
	else
		DELAY_MS(75);	
		
	msk=0x0001;
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
			if(command == 0x03)
			{
				SHTbuff[y].TEMP = (uint16_t)SHT_rcv8_t(y,0x00) << 8;
				SHTbuff[y].TEMP |= (uint16_t)SHT_rcv8_t(y,0x00);
			}
			else
			{
				SHTbuff[y].HUM = (uint16_t)SHT_rcv8_t(y,0x00) << 8;
				SHTbuff[y].HUM |= (uint16_t)SHT_rcv8_t(y,0x00);
			}
			crc = SHT_rcv8_t(y,0x01);
		}
		msk = msk<<4;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_meas_dummy(void)
{
	uint16_t msk=0x0001;
	uint8_t y;
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
		SHT_connection_reset(y);
		SHT_TRX_start(y);
		SHT_send8_t(0x03,y);
//		SHT_wait(y);
		DELAY_MS(220);
		SHT_rcv8_t(y,0x00);
		SHT_rcv8_t(y,0x01);
		}
		msk = msk<<4;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_read_stat(void)
{
	uint8_t y;
	uint16_t msk=0x0001;
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
//		SHT_connection_reset(y);
		SHT_TRX_start(y);
		SHT_send8_t(SHT_READ_STAT, y);
//		SHT_wait(y);
		SHTbuff[y].STATUS = SHT_rcv8_t(y, 0x01);
		}
		msk = msk<<4;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_write_stat(uint8_t status)
{
	uint8_t y;
	uint16_t msk=0x0001;
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
		SHT_TRX_start(y);
		SHT_send8_t(SHT_WRITE_STAT, y);
		SHT_sw_data_dir(y,0x01);
		delay_us(1);
		SHT_send8_t(status, y);
		}
		msk = msk<<4;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_reset(void)
{
	uint8_t y;
	uint16_t msk=0x0005;
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
			SHT_sw_data_dir(y,0x01);
			SHT_connection_reset(y);
			delay_us(1);
			SHT_TRX_start(y);
			if(SHT_send8_t(SHT_RESET, y))
			{
				mask.SHTM &= ~msk;			//vymazanie z masky ak nebolo ack...
			}
		}
		msk = msk<<4;
	}
	DELAY_MS(50);
//	SHT_read_stat();
	return;
}

//------------------------------------------------------------------------------

void SHT_connection_reset(uint8_t y)
{
	uint8_t x;
	
	for(x=0;x<10;x++)
	{
		SHT_clk(y);
	}
	return;
}

//------------------------------------------------------------------------------

uint8_t SHT_send8_t(uint8_t data, uint8_t y)
{
	uint8_t x;
	
	for(x=0;x<8;x++)
	{
		SHT_send_bit((data&0x80),y);
		data=data<<1;
	}
	SHT_sw_data_dir(y,0x00);
//	delay_us(1);
	return(SHT_rcv_bit(y));
}

//------------------------------------------------------------------------------

uint8_t SHT_rcv8_t(uint8_t y, uint8_t ack)		//ack = 1 - no ACK, terminate comm; ack=0 send ACK continue comm.
{
	uint8_t data=0, x;
	
	for(x=0;x<8;x++)
	{
		data = data<<1;
		data |= SHT_rcv_bit(y);
	}
	SHT_sw_data_dir(y,0x01);
	delay_us(1);
	SHT_send_bit(ack,y);
	SHT_sw_data_dir(y,0x00);
	return(data);
}

//------------------------------------------------------------------------------

void SHT_TRX_start(uint8_t y)
{

	SHT_PORT.DIR |= SDA_bm;
	switch(y)
	{
		case 0x00:
				SHT_PORT.OUTSET = SCL0;		//clk Hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SDA_bm;	// data lo
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SCL0;		// clk lo
				delay_us(4);	// wait 2us
				SHT_PORT.OUTSET = SCL0;		//clk hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTSET = SDA_bm;	// data hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SCL0;		// clk lo
				break;
		case 0x01:
				SHT_PORT.OUTSET = SCL1;		//clk Hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SDA_bm;	// data lo
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SCL1;		// clk lo
				delay_us(4);	// wait 2us
				SHT_PORT.OUTSET = SCL1;		//clk hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTSET = SDA_bm;	// data hi
				delay_us(1);	// wait 1us
				SHT_PORT.OUTCLR = SCL1;		// clk lo
				break;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_send_bit(uint8_t data, uint8_t y)
{
	if(!(data))
	{
		SHT_PORT.OUTCLR = SDA_bm;		// DATA lo
	}
	else
	{
		SHT_PORT.OUTSET = SDA_bm;		// DATA hi
	}
	SHT_clk(y);
	return;
}

//------------------------------------------------------------------------------

void SHT_clk(uint8_t y)
{
	delay_us(2);
	switch(y)
	{
		case 0x00:
				SHT_PORT.OUTSET = SCL0;	// CLK hi
				delay_us(2);			// wait 1us
				SHT_PORT.OUTCLR = SCL0;	// CLK lo
				break;
		case 0x01:
				SHT_PORT.OUTSET = SCL1;	// CLK hi
				delay_us(2);			// wait 1us
				SHT_PORT.OUTCLR = SCL1;	// CLK lo
				break;
	}
	return;
}

//------------------------------------------------------------------------------

uint8_t SHT_rcv_bit(uint8_t y)
{
	uint8_t data=0;
	
	switch(y)
	{
		case 0x00:
			delay_us(5);	// wait 1us
			SHT_PORT.OUTSET = SCL0;	// CLK hi
			data = SHT_PORT.IN & SDA_bm
			delay_us(2);	// wait 1us
			SHT_PORT.OUTCLR = SCL0;	// CLK lo
			break;
		case 0x01:
			delay_us(5);	// wait 1us
			SHT_PORT.OUTSET = SCL1;	// CLK hi
			data = SHT_PORT.IN & SDA_bm
			delay_us(2);	// wait 1us
			SHT_PORT.OUTCLR = SCL1;	// CLK lo
			break;
	}
	return(data);
}

//------------------------------------------------------------------------------

/*
void SHT_send_ack(uint8_t y, uint8_t ack)
{
	SHT_send_bit(ack, y);
	return;
}

//------------------------------------------------------------------------------

uint8_t SHT_rcv_ack(uint8_t y)
{
	uint8_t error=0;
	if(SHT_rcv_bit(y))
	{
		error=1;
	}
	return(error);
}
*/

//------------------------------------------------------------------------------

void SHT_wait(uint8_t y)
{
	switch(y)
	{
#ifdef DS_3
		case 0x00:
			while(PINE & 0x04);	// if DATA hi
			break;
		case 0x01:
#else
		case 0x00:
			while(PINF & 0x01);	// if DATA hi
			break;
		case 0x01:
			while(PINF & 0x10);	// if DATA hi
#endif  //DS_3
			break;
		case 0x02:
			while(PINA & 0x01);	// if DATA hi
			break;
		case 0x03:
			while(PINA & 0x10);	// if DATA hi
			break;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_init(void)
{
	uint16_t msk = mask.SHTM;
	if(msk && 0x0001)

#ifdef DS_#
	{
		DDRE  |= 0x04;			//pull up + input for data,
		DDRE  &= 0xFD;			//out 0 for SCK
		PORTE &= 0xFB;
		PORTE |= 0x02;
	}
	msk >>= 4;
#else
 	{
		DDRF  |= 0x08;			//pull up + input for data, 
		DDRF  &= 0xFE;			//out 0 for SCK
		PORTF &= 0xF7;
		PORTF |= 0x01;
	}
	msk >>= 4;
	if(msk && 0x0001)
	{
		DDRF  &= 0xEF;			//pull up + input for data
		PORTF |= 0x10;
		DDRB  |= 0x80;		//0 + output for SCK
		PORTB &= 0x7F;
	}
#endif  //DS_3

	msk >>= 4;
	if(msk && 0x0001)
	{
		DDRA  |= 0x08;			//pull up + input for data
		DDRA  &= 0xFE;			//out 0 for SCK
		PORTA &= 0xF7;
		PORTA |= 0x01;
	}
	msk >>= 4;
	if(msk && 0x0001)
	{
		DDRA  |= 0x80;			//pull up + output for SCK & data
		DDRA  &= 0xEF;			//out 0 for SCK
		PORTA &= 0x7F;
		PORTA |= 0x10;
	}
	DELAY_MS(20);
	SHT_reset();
	return;
}




//------------------------------------------------------------------------------

void SHT_sw_data_dir(uint8_t y, uint8_t dir)
{
	switch(y)
	{
#ifdef DS_3
		case 0x00:
			if(dir)
			{
				DDRE  |= 0x04;			//output for data,
			}
			else
			{
				DDRE  &= 0xFB;			//pull up + input for data,
				PORTE |= 0x04;			//
			}
			break;
		case 0x01:
#else
		case 0x00:
			if(dir)
			{
				DDRF  |= 0x01;			//output for data, 
			}
			else
			{
				DDRF  &= 0xFE;			//pull up + input for data, 
				PORTF |= 0x01;			//
			}
			break;
		case 0x01:
			if(dir)
			{
				DDRF  |=0x10;		//output for data
			}
			else
			{
				DDRF  &= 0xEF;			//pull up + input for data
				PORTF |= 0x10;
			}
#endif  //DS_3
			break;
		case 0x02:
			if(dir)
			{
				DDRA  |= 0x01;
			}
			else
			{
				DDRA  &= 0xFE;
				PORTA |= 0x01;
			}
			break;
		case 0x03:
			if(dir)
			{
				DDRA  |= 0x10;
			}
			else
			{
				DDRA  &= 0xEF;
				PORTA |= 0x10;
			}
			break;
	}
	return;
}
