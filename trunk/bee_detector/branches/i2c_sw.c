#include <avr/io.h>
#include <inttypes.h>
#include <avr/delay.h>

//#include "1wire.h"
#include "include/hw.h"
#include "include/i2c_sw.h"
#include "include/routines.h"


//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//



//------------------------------------------------------------------------------

void I2C_connection_reset(uint8_t y)
{
	uint8_t x;
	
	for(x=0;x<10;x++)
	{
		I2C_clk(y);
	}
	return;
}

//------------------------------------------------------------------------------

uint8_t I2C_send8_t(uint8_t data, uint8_t y)		//converted
{
	uint8_t x;
	
	for(x=0;x<8;x++)
	{
		I2C_send_bit((data&0x80),y);
		data=data<<1;
	}
	I2C_sw_data_dir(y,0x00);
//	delay_us(1);
	return(I2C_rcv_bit(y));		//read ACK
}

//------------------------------------------------------------------------------

uint8_t I2C_rcv8_t(uint8_t y, uint8_t ack, uint8_t *data)		//ack = 1 - no ACK, terminate comm; ack=0 send ACK continue comm.
{							//converted
	uint8_t d, x, z;
	
	for(x=0;x<8;x++)
	{
		d = I2C_rcv_bit(y);
		for(z=0; z<8;z++)
		{
			data[z] = data[z]<<1;
			data[z] |= (d & 0x01);
			d = d>>1;
		}
	}
	I2C_sw_data_dir(y,0x01);
	_delay_us(2);
	I2C_send_bit(ack,y);
	I2C_sw_data_dir(y,0x00);
	return(data);
}

//------------------------------------------------------------------------------

void I2C_TRX_start(uint8_t y)	//converted
{
	uint8_t scl;

	I2C_PORT.DIR |= SDA_bm;
	
	if(y)
		scl = SCL1;
	else
		scl = SCL0;

	I2C_PORT.OUTSET = scl;		//clk Hi
	_delay_us(2);	// wait 1us
	I2C_PORT.OUTCLR = SDA_bm;	// data lo
	_delay_us(2);	// wait 1us
	I2C_PORT.OUTCLR = scl;		// clk lo
	_delay_us(8);	// wait 2us
	I2C_PORT.OUTSET = scl;		//clk hi
	_delay_us(2);	// wait 1us
	I2C_PORT.OUTSET = SDA_bm;	// data hi
	_delay_us(2);	// wait 1us
	I2C_PORT.OUTCLR = scl;		// clk lo
	return;
}

//------------------------------------------------------------------------------

void I2C_send_bit(uint8_t data, uint8_t y)	//converted
{
	if(!(data))
	{
		I2C_PORT.OUTCLR = SDA_bm;		// DATA lo
	}
	else
	{
		I2C_PORT.OUTSET = SDA_bm;		// DATA hi
	}
	I2C_clk(y);
	return;
}

//------------------------------------------------------------------------------

void I2C_clk(uint8_t y)					//converted
{
	uint8_t scl;
	
	_delay_us(4);

	if(y)
		scl = SCL1;
	else
		scl = SCL0;

	I2C_PORT.OUTSET = scl;	// CLK hi
	_delay_us(4);			// wait 1us
	I2C_PORT.OUTCLR = scl;	// CLK lo
	return;
}

//------------------------------------------------------------------------------

uint8_t I2C_rcv_bit(uint8_t y)			//converted
{
	uint8_t scl;
	uint8_t data=0;

	if(y)
		scl = SCL1;
	else
		scl = SCL0;
	
	_delay_us(10);	// wait 1us
	I2C_PORT.OUTSET = scl;	// CLK hi
	data = I2C_PORT.IN & SDA_bm;
	_delay_us(4);	// wait 1us
	I2C_PORT.OUTCLR = scl;	// CLK lo
	return(data);
}

//------------------------------------------------------------------------------

/*
void I2C_send_ack(uint8_t y, uint8_t ack)
{
	I2C_send_bit(ack, y);
	return;
}

//------------------------------------------------------------------------------

uint8_t I2C_rcv_ack(uint8_t y)
{
	uint8_t error=0;
	if(I2C_rcv_bit(y))
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
			while(I2C_PORT.IN & Mask.SHT0_bm);	// if DATA hi
			break;
		case 1:
			while(I2C_PORT.IN & Mask.SHT1_bm);	// if DATA hi
			break;
	}
	return;
}

//------------------------------------------------------------------------------

void I2C_init(void)				//converted
{
	PORTCFG.MPCMASK = SDA_bm;
//	I2C_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;	// wired-and and pull-up set for all SDA lines (should be output???)
	I2C_PORT.PIN0CTRL = PORT_OPC_PULLUP_gc;	// wired-and and pull-up set for all SDA lines (should be output???)
	I2C_PORT.DIRCLR  = SDA_bm;		//???,
	I2C_PORT.DIRSET  = SCL_bm;		//out 0 for SCK
	I2C_PORT.OUTCLR = SCL_bm;

	_delay_ms(20);
	I2C_reset();
	return;
}




//------------------------------------------------------------------------------

void I2C_sw_data_dir(uint8_t y, uint8_t dir)		//converted
{	//podla masky - SHT, ktore nemeraju by mali byt LO? kontrola vo SHT_wait - tu neriesit!
	// kedze wired and cfg je pouzita, smer netreba prepinat vobec... staci poslat data = 1
	
//	SHT_PORT.OUTSET = SDA_bm;
	

			if(dir)
			{
				I2C_PORT.DIRSET = SDA_bm;			//output for data,
			}
			else
			{
				I2C_PORT.DIRCLR = SDA_bm;			//pull up + input for data,
			}
	return;
}
