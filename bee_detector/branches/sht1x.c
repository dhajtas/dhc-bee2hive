#include <avr/io.h>
#include <inttypes.h>

//#include "1wire.h"
#include "hw.h"
#include "sht1x.h"
#include "routines.h"


//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//uint8_t SHTmask;		mask.SHTM instead
SHT_t SHTbuff[12];

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
	uint8_t data[8];
	
	for(y=0;y<4;y++)
	{
		if(mask.SHTM && msk)
		{
		SHT_connection_reset(y);
		SHT_TRX_start(y);
		SHT_send8_t(0x03,y);
//		SHT_wait(y);
		DELAY_MS(220);
		SHT_rcv8_t(y,0x00,data);
		SHT_rcv8_t(y,0x01,data);
		}
		msk = msk<<4;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_read_stat(void)			//converted
{
	uint8_t y,i,j=0;
	uint8_t data[8];
	uint16_t msk=0x0001;
	
	for(y=0;y<2;y++)
	{
//		SHT_connection_reset(y);
		SHT_TRX_start(y);
		SHT_send8_t(SHT_READ_STAT, y);
//		SHT_wait(y);
		SHT_rcv8_t(y, 0x01, data);
		for(i=0;i<8;i++)
		{
			switch(i)
			{
				case 1:
				case 6:
						break;
				default:
						j++;
			}
			if(Mask.SHT & (0x0001<<(j-1)))
				SHTbuff[j-1].STATUS = data[i];
		}		
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_write_stat(uint8_t status)
{
	uint8_t y;
	uint16_t msk=0x0001;
	
	for(y=0;y<2;y++)
	{
		SHT_TRX_start(y);
		SHT_send8_t(SHT_WRITE_STAT, y);
//??		SHT_sw_data_dir(y,0x01);
		delay_us(1);
		SHT_send8_t(status, y);
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_reset(void)		//converted
{
	uint8_t y, x, d;
	uint16_t msk=0x0001;

	Mask.SHT = Mask.MIC;
	
	for(y=0;y<2;y++)			//SHT autodetect ?
	{
//??			SHT_sw_data_dir(y,0x01);
			SHT_connection_reset(y);
			delay_us(1);
			SHT_TRX_start(y);
			d = SHT_send8_t(SHT_RESET, y)
			for(x=0;x<8;x++)
			{
				switch(x)
				{
					case 1:
					case 6:
							break;
					default:
							if((d & 0x01))	//if ACK not received
								Mask.SHT & = ~msk;		//vymaz pripadny bit z masky
							msk = msk<<1;
							break;	
				}					
				d = d >> 1
			}
	}

	generate_mask_bm();
	
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

uint8_t SHT_send8_t(uint8_t data, uint8_t y)		//converted
{
	uint8_t x;
	
	for(x=0;x<8;x++)
	{
		SHT_send_bit((data&0x80),y);
		data=data<<1;
	}
//??	SHT_sw_data_dir(y,0x00);
//	delay_us(1);
	return(SHT_rcv_bit(y));		//read ACK
}

//------------------------------------------------------------------------------

uint8_t SHT_rcv8_t(uint8_t y, uint8_t ack, uint8_t *data)		//ack = 1 - no ACK, terminate comm; ack=0 send ACK continue comm.
{							//converted
	uint8_t d, x, y;
	
	for(x=0;x<8;x++)
	{
		d = SHT_rcv_bit(y);
		for(y=0; y<8;y++)
		{
			data[y] = data[y]<<1;
			data[y] |= (d & 0x01);
			d = d>>1;
		}
	}
//??	SHT_sw_data_dir(y,0x01);
	delay_us(1);
	SHT_send_bit(ack,y);
//??	SHT_sw_data_dir(y,0x00);
	return(data);
}

//------------------------------------------------------------------------------

void SHT_TRX_start(uint8_t y)	//converted
{
	uint8_t scl;

	SHT_PORT.DIR |= SDA_bm;
	
	if(y)
		scl = SCL1;
	else
		scl = SCL0;

	SHT_PORT.OUTSET = scl;		//clk Hi
	delay_us(1);	// wait 1us
	SHT_PORT.OUTCLR = SDA_bm;	// data lo
	delay_us(1);	// wait 1us
	SHT_PORT.OUTCLR = scl;		// clk lo
	delay_us(4);	// wait 2us
	SHT_PORT.OUTSET = scl;		//clk hi
	delay_us(1);	// wait 1us
	SHT_PORT.OUTSET = SDA_bm;	// data hi
	delay_us(1);	// wait 1us
	SHT_PORT.OUTCLR = scl;		// clk lo
	return;
}

//------------------------------------------------------------------------------

void SHT_send_bit(uint8_t data, uint8_t y)	//converted
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

void SHT_clk(uint8_t y)					//converted
{
	uint8_t scl;
	
	delay_us(2);

	if(y)
		scl = SCL1;
	else
		scl = SCL0;

	SHT_PORT.OUTSET = scl;	// CLK hi
	delay_us(2);			// wait 1us
	SHT_PORT.OUTCLR = scl;	// CLK lo
	return;
}

//------------------------------------------------------------------------------

uint8_t SHT_rcv_bit(uint8_t y)			//converted
{
	uint8_t scl;
	uint8_t data=0;

	if(y)
		scl = SCL1;
	else
		scl = SCL0;
	
	delay_us(5);	// wait 1us
	SHT_PORT.OUTSET = scl;	// CLK hi
	data = SHT_PORT.IN & SDA_bm
	delay_us(2);	// wait 1us
	SHT_PORT.OUTCLR = scl;	// CLK lo
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

void SHT_wait(uint8_t y)			//converted
{
	switch(y)	// SHT ktore nie su by nemali byt kontrolovane! - data budu stale High...(zabezpecit v inite alebo tu?)
	{
		case 0:
			while(SHT_PORT.IN & Mask.SHT0_bm);	// if DATA hi
			break;
		case 1:
			while(SHT_PORT.IN & Mask.SHT1_bm);	// if DATA hi
			break;
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_init(void)				//converted
{
	uint16_t msk = mask.SHTM;
	if(msk && 0x0001)

	{
		PORTCFG.MPCMASK = SDA_bm;
		SHT_PORT.PIN0CTRL = WIREDANDPULL_gc;	// wired-and and pull-up set for all SDA lines (should be output???)
		SHT_PORT.DIRSET  = SDA_bm;		//???,
		SHT_PORT.DIRSET  = SCL_bm;		//out 0 for SCK
		SHT_PORT.OUTCLR = SCL_bm;
	}
	DELAY_MS(20);
	SHT_reset();
	return;
}




//------------------------------------------------------------------------------

void SHT_sw_data_dir(uint8_t y, uint8_t dir)		//converted
{	//podla masky - SHT, ktore nemeraju by mali byt LO? kontrola vo SHT_wait - tu neriesit!
	// kedze wired and cfg je pouzita, smer netreba prepinat vobec... staci poslat data = 1
	
	SHT_PORT.OUTSET = SDA_bm;
	
	/*
	switch(y)
	{
		case 0:
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
		case 1:
			if(dir)
			{
				DDRF  |=0x10;		//output for data
			}
			else
			{
				DDRF  &= 0xEF;			//pull up + input for data
				PORTF |= 0x10;
			}
			break;
	}
	*/
	return;
}
