
void SHT_TRX_start(uint8_t y)
{
	switch(y)
	{
		case 0x00:	
		//	PORTF |= 0x01; 	// DATA 1, pull-up
			DDRE  |= 0x04;	// DATA outport  PE3
			PORTE |= 0x02;	// CLK hi        PE2
			delay_us(1);	// wait 1us
			PORTE &= 0xFB;	// DATA lo
			delay_us(1);	// wait 1us
			PORTE &= 0xFD;	// CLK lo
			delay_us(4);	// wait 2us
			PORTE |= 0x02;	// CLK hi
			delay_us(1);	// wait 1us
			PORTE |= 0x04;	// DATA hi
			delay_us(1);	// wait 1us
			PORTE &= 0xFD;	// CLK lo
			break;
		case 0x01:
//			DDRF  |= 0x10;	// DATA outport
//			PORTB |= 0x80;	// CLK hi
//			delay_us(1);	// wait 1us
//			PORTF &= 0xEF;	// DATA lo
//			delay_us(1);	// wait 1us
//			PORTB &= 0x7F;	// CLK lo
//			delay_us(4);	// wait 2us
//			PORTB |= 0x80;	// CLK hi
//			delay_us(1);	// wait 1us
//			PORTF |= 0x10;	// DATA hi
//			delay_us(1);	// wait 1us
//			PORTB &= 0x7F;	// CLK lo
			break;
		case 0x02:	
			DDRF  |= 0x01;	// DATA outport
			PORTF |= 0x08;	// CLK hi
			delay_us(1);	// wait 1us
			PORTF &= 0xFE;	// DATA lo
			delay_us(1);	// wait 1us
			PORTF &= 0xF7;	// CLK lo
			delay_us(4);	// wait 2us
			PORTF |= 0x08;	// CLK hi
			delay_us(1);	// wait 1us
			PORTF |= 0x01;	// DATA hi
			delay_us(1);	// wait 1us
			PORTF &= 0xF7;	// CLK lo
			break;
		case 0x03:	
			DDRF  |= 0x10;	// DATA outport
			PORTF |= 0x80;	// CLK hi
			delay_us(1);	// wait 1us
			PORTF &= 0xEF;	// DATA lo
			delay_us(1);	// wait 1us
			PORTF &= 0x7F;	// CLK lo
			delay_us(4);	// wait 2us
			PORTF |= 0x80;	// CLK hi
			delay_us(1);	// wait 1us
			PORTF |= 0x10;	// DATA hi
			delay_us(1);	// wait 1us
			PORTF &= 0x7F;	// CLK lo
			break;
	}
	return;
}

void SHT_send_bit(uint8_t data, uint8_t y)
{
	switch(y)
	{
		case 0x00:
			if(!(data))
			{
				PORTE &= 0xFB;		// DATA lo
			}
			else
			{
				PORTE |= 0x04;		// DATA hi
			}
			delay_us(1);	// wait 1us
			PORTE |= 0x02;	// CLK hi
			delay_us(2);	// wait 1us
			PORTE &= 0xFD;	// CLK lo
			break;
		case 0x01:
//			if(!(data))
//			{
//				PORTF &= 0xEF;		// DATA lo
//			}
//			else
//			{
//				PORTF |= 0x10;		// DATA hi
//			}
//			delay_us(1);	// wait 1us
//			PORTB |= 0x80;	// CLK hi
//			delay_us(2);	// wait 1us
//			PORTB &= 0x7F;	// CLK lo
			break;
		case 0x02:
			if(!(data))
			{
				PORTF &= 0xFE;		// DATA lo
			}
			else
			{
				PORTF |= 0x01;		// DATA hi
			}
			delay_us(1);	// wait 1us
			PORTF |= 0x08;	// CLK hi
			delay_us(2);	// wait 1us
			PORTF &= 0xF7;	// CLK lo
			break;
		case 0x03:
			if(!(data))
			{
				PORTF &= 0xEF;		// DATA lo
			}
			else
			{
				PORTF |= 0x10;		// DATA hi
			}
			delay_us(1);	// wait 1us
			PORTF |= 0x80;	// CLK hi
			delay_us(2);	// wait 1us
			PORTF &= 0x7F;	// CLK lo
			break;
	}
	return;
}

uint8_t SHT_rcv_bit(uint8_t y)
{
	uint8_t data=0;
	
	switch(y)
	{
		case 0x00:
			delay_us(5);	// wait 1us
			PORTE |= 0x02;	// CLK hi
			if(PINE & 0x04)	// if DATA hi
			{
				data = 0x01;		
			}
			delay_us(2);	// wait 1us
			PORTE &= 0xFD;	// CLK lo
			break;
		case 0x01:
//			delay_us(5);	// wait 1us
//			PORTB |= 0x80;	// CLK hi
//			if(PINF & 0x10)	// if DATA hi
//			{
//				data = 0x01;
//			}
//			delay_us(2);	// wait 1us
//			PORTB &= 0x7F;	// CLK lo
			break;
		case 0x02:
			delay_us(5);	// wait 1us
			PORTF |= 0x08;	// CLK hi
			if(PINF & 0x01)	// if DATA hi
			{
				data = 0x01;
			}
			delay_us(2);	// wait 1us
			PORTF &= 0xF7;	// CLK lo
			break;
		case 0x03:
			delay_us(5);	// wait 1us
			PORTF |= 0x80;	// CLK hi
			if(PINF & 0x10)	// if DATA hi
			{
				data = 0x01;		
			}
			delay_us(2);	// wait 1us
			PORTF &= 0x7F;	// CLK lo
			break;
	}
	return(data);
}

/*
void SHT_send_ack(uint8_t y, uint8_t ack)
{
	SHT_send_bit(ack, y);
	return;
}

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

void SHT_wait(uint8_t y)
{
	switch(y)
	{
		case 0x00:
			while(PINE & 0x04);	// if DATA hi
			break;
		case 0x01:
//			while(PINF & 0x10);	// if DATA hi
			break;
		case 0x02:
			while(PINF & 0x01);	// if DATA hi
			break;
		case 0x03:
			while(PINF & 0x10);	// if DATA hi
			break;
	}
	return;
}

void SHT_init(void)
{
	uint16_t msk = mask.SHTM;
	if(msk && 0x0001)
	{
		DDRE  |= 0x04;			//pull up + input for data,
		DDRE  &= 0xFD;			//out 0 for SCK
		PORTE &= 0xFB;
		PORTE |= 0x02;
	}
	msk >>= 4;
	if(msk && 0x0001)
	{
//		DDRF  &= 0xEF;			//pull up + input for data
//		PORTF |= 0x10;
//		DDRB  |= 0x80;		//0 + output for SCK
//		PORTB &= 0x7F;
	}
	msk >>= 4;
	if(msk && 0x0001)
	{
		DDRF  |= 0x08;			//pull up + input for data
		DDRF  &= 0xFE;			//out 0 for SCK
		PORTF &= 0xF7;
		PORTF |= 0x01;
	}
	msk >>= 4;
	if(msk && 0x0001)
	{
		DDRF  |= 0x80;			//pull up + output for SCK & data
		DDRF  &= 0xEF;			//out 0 for SCK
		PORTF &= 0x7F;
		PORTF |= 0x10;
	}
	DELAY_MS(20);
	SHT_reset();
	return;
}

void SHT_clk(uint8_t y)
{
	switch(y)
	{
		case 0x00:
			delay_us(2);	// wait 2us
			PORTE |= 0x02;	// CLK hi
			delay_us(2);	// wait 2us
			PORTE &= 0xFD;	// CLK lo
			break;
		case 0x01:
//			delay_us(2);	// wait 2us
//			PORTB |= 0x80;	// CLK hi
//			delay_us(2);	// wait 2us
//			PORTB &= 0x7F;	// CLK lo
			break;
		case 0x02:
			delay_us(2);	// wait 2us
			PORTF |= 0x08;	// CLK hi
			delay_us(2);	// wait 2us
			PORTF &= 0xF7;	// CLK lo
			break;
		case 0x03:
			delay_us(2);	// wait 2us
			PORTF |= 0x80;	// CLK hi
			delay_us(2);	// wait 2us
			PORTF &= 0x7F;	// CLK lo
			break;
	}
	return;
}

void SHT_sw_data_dir(uint8_t y, uint8_t dir)
{
	switch(y)
	{
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
//			if(dir)
//			{
//				DDRF  |=0x10;		//output for data
//			}
//			else
//			{
//				DDRF  &= 0xEF;			//pull up + input for data
//				PORTF |= 0x10;
//			}
			break;
		case 0x02:
			if(dir)
			{
				DDRF  |= 0x01;
			}
			else
			{
				DDRF  &= 0xFE;
				PORTF |= 0x01;
			}
			break;
		case 0x03:
			if(dir)
			{
				DDRF  |= 0x10;
			}
			else
			{
				DDRF  &= 0xEF;
				PORTF |= 0x10;
			}
			break;
	}
	return;
}
