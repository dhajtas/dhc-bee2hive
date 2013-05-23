#include <avr/io.h>
#include <inttypes.h>
#include <avr/delay.h>

//#include "1wire.h"
#include "include/hw.h"
#include "include/sht1x.h"
#include "include/i2c_sw.h"
#include "include/routines.h"


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
	uint8_t y, i, j=0;
	uint8_t crc[8], data0[8], data1[8];
	
	for(y=0;y<2;y++)
	{
//		SHT_connection_reset(y);
		I2C_TRX_start(y);
		I2C_send8_t(command,y);
//		SHT_wait(y);
		
		if(command == SHT_MEAS_TEMP)
			_delay_ms(220);
		else
			_delay_ms(75);	

		I2C_rcv8_t(y,0,data1);
		I2C_rcv8_t(y,0,data0);
		I2C_rcv8_t(y,1,crc);

		msk=0x0001;

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
			{
				if(command == SHT_MEAS_TEMP)
					SHTbuff[j-1].TEMP = (uint16_t)data1[i]<<8 | data0[i];
				else
					SHTbuff[j-1].HUM = (uint16_t)data1[i]<<8 | data0[i];
			}
		}
	}	
	return;
}

//------------------------------------------------------------------------------

void SHT_meas_dummy(void)
{
	uint8_t y;
	uint8_t data[8];
	
	for(y=0;y<2;y++)
	{
		if(Mask.SHT && (0x003F<<y))
		{
		I2C_connection_reset(y);
		I2C_TRX_start(y);
		I2C_send8_t(SHT_MEAS_TEMP,y);
//		SHT_wait(y);
		_delay_ms(220);
		I2C_rcv8_t(y,0x00,data);
		I2C_rcv8_t(y,0x01,data);
		}
	}
	return;
}

//------------------------------------------------------------------------------

void SHT_read_stat(void)			//converted
{
	uint8_t y,i,j=0;
	uint8_t data[8];
	
	for(y=0;y<2;y++)
	{
//		SHT_connection_reset(y);
		I2C_TRX_start(y);
		I2C_send8_t(SHT_READ_STAT, y);
//		SHT_wait(y);
		I2C_rcv8_t(y, 0x01, data);
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
	
	for(y=0;y<2;y++)
	{
		I2C_TRX_start(y);
		I2C_send8_t(SHT_WRITE_STAT, y);
		I2C_sw_data_dir(y,0x01);
		_delay_us(1);
		I2C_send8_t(status, y);
	}
	return;
}

//------------------------------------------------------------------------------

uint8_t SHT_reset(uint8_t y)		//converted
{
	uint8_t d;


	
	I2C_sw_data_dir(y,0x01);
	I2C_connection_reset(y);
	_delay_us(2);
	I2C_TRX_start(y);
	d = I2C_send8_t(SHT_RESET, y);

	
	_delay_ms(50);
//	SHT_read_stat();
	return(d);
}



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

