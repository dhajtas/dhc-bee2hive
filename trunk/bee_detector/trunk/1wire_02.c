
//-----------------------------------------------------------

void ow_sendbit(register uint16_t dallas,register uint8_t data)
{
 register uint8_t pomx = 0,pom2,pom1,pom20,pom10;
	
 	pom1 = PORTDIO;
 	pom10 = pom1 & ~((uint8_t)dallas);
 	pom2 = PORTADC;
 	pom20 = pom2 & ~((uint8_t)(dallas>>8) & 0x7F);
	if(dallas & 0x8000)
	{
		pomx = 1;
		cbi(PORTB,7);
	}
	PORTDIO = pom10;			//clear 1w lines low byte
	PORTADC = pom20;			//high byte

	cli();	

//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	DELAY_US(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	if(data & 0x01)				//				!data.0
	{
		PORTDIO = pom1;			//if send 1, set the lines
		PORTADC = pom2;
		if(pomx)
			sbi(PORTB,7);
	}

	delay_us(ONE_WIRE_WRITE_SLOT_TIME_US);
	
	sei();
	PORTDIO = pom1;				// set the 1W lines
	PORTADC = pom2;
	if(pomx)
		sbi(PORTB,7);
	
}

//-----------------------------------------------------------

uint16_t ow_recbit(register uint16_t dallas)
{
 register uint8_t pomx = 0,pom4,pom3,pom2,pom1,pom10,pom20,pom30,pom40;

 	pom1 = PORTDIO;
 	pom10 = pom1 & ~((uint8_t)dallas);
 	pom2 = PORTADC;
 	pom20 = pom2 & ~((uint8_t)(dallas>>8) & 0x7F);
 	pom3 = DDRDIO;
 	pom30 = pom3 & ~((uint8_t)dallas);
	pom4 = DDRADC;
	pom40 = pom4 & ~((uint8_t)(dallas>>8) & 0x7F);
	
	cli();					//disable interrupts (time critical)
	
	if(dallas & 0x8000)
	{
		pomx = 1;
		cbi(PORTB,7);
	}
	PORTDIO = pom10;			//clear 1w lines low byte
	PORTADC = pom20;			//high byte
	
//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	DELAY_US(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	PORTDIO = pom1;				//active pull-up najprv vnutit tvrdu jeddnotku ako vystup 
	DDRDIO = pom30;				//a potom prepnut na vstup s pull-up odporom
	PORTADC = pom2;
	DDRADC = pom40;

	
	if(pomx)
	{
		sbi(PORTB,7);	
		cbi(DDRB,7);
	}
	
	DELAY_US(13);
	
	pom1 = PINDIO;  			//low byte
	pom2 = PINADC;				//high byte
	pom30 = PORTB & 0x80;
	sei();
	pom2 = (pom2 & 0x7F) | pom30;

	delay_us(ONE_WIRE_READ_SLOT_TIME_US);

	DDRDIO = pom3;				//return ports to init state for 1w comm (outpu, H on pin)
	DDRADC = pom4;
	if(pomx)
		sbi(DDRB,7);
	return((uint16_t)pom1 | ((uint16_t)pom2)<<8);	//combine low and high byte to one 16bit int
}

//-----------------------------------------------------------

uint16_t ow_reset(uint16_t dallas)
{

 register uint8_t pomx = 0,pom4,pom3,pom2,pom1,dallas_low,dallas_high;
 uint8_t pom20,pom10;

	dallas_low = (uint8_t)dallas;
	dallas_high = (uint8_t)(dallas>>8) & 0x7F;
 	pom10 = PORTDIO;
 	pom20 = PORTADC;
	pom3 = DDRDIO;
	pom4 = DDRADC;
 	pom1 = pom10;
 	pom2 = pom20;
	PORTDIO = pom1 | dallas_low;		//set 1w lines low byte
	PORTADC = pom2 | dallas_high;		//high byte
	if(dallas & 0x8000)
	{
		pomx = 1;
		sbi(PORTB,7);
	}
	delay_us(250);				//na odstranenie dummy resetu (pred tymto boli draty na 0 takze hned pride pulz a potom sa uz nic nedeje :-( )
	DDRDIO = pom3 | dallas_low;		//set 1w lines as output low byte
	DDRADC = pom4 | dallas_high;		//high byte
	if(pomx)
		sbi(DDRB,7);

	PORTDIO = pom1 & ~dallas_low;			//clear 1w lines low byte
	PORTADC = pom2 & ~dallas_high;			//high byte
	if(pomx)
		cbi(PORTB,7);
	delay_us(550);

	cli();							//disable interrupts
	DDRDIO = pom3 & ~dallas_low;			//inputs 1w (low byte)
	PORTDIO = pom1 | dallas_low;			//set 1w lines low byte (passive pull-up)
	DDRADC = pom4 & ~dallas_high;			//inputs 1w (high byte)
	PORTADC = pom2 | dallas_high;			//set 1w lines high byte (passive pull-up)
	if(pomx)
	{
		cbi(DDRB,7);
		sbi(PORTB,7);
	}

	delay_us(70);

	pom1 = PINDIO;				//citaj presence pulse -> 0 znamena pritomny
	pom2 = (PINADC & 0x7F) | (PORTB & 0x80);

	delay_us(300);
	
	pom1 = pom1 | ~PINDIO;			//ak tam skutocne je, 0 musi po case zmiznut.
	pom2 = pom2 | ~((PINADC & 0x7F) | (PORTB & 0x80));			// ak nezmizne, tak sa povodna maska upravi
	
	sei();			//enable interrupts

	delay_us(250);
	pom1 = ~pom1;
	pom2 = ~pom2;

	DDRDIO = (pom3 | pom1);		//vrat port do povodneho stavu, ale kde najde 1w ostane vystup!
	PORTDIO = (pom10 | pom1);		//vrat port do povodneho stavu, ale kde je 1w tam bude H! 
	DDRADC = (pom4 | pom2);
	PORTADC = (pom20 | pom2);

	return((uint16_t)pom1 | ((uint16_t)pom2<<8));			//inverted result is in zl
}

//-----------------------------------------------------------

void ow_rstprt(register uint16_t dallas)
{
 register uint8_t ndallas_low = ~(uint8_t)dallas;
 register uint8_t ndallas_high = ~(uint8_t)(dallas>>8);
 
	DDRDIO &= ndallas_low;		//put all 1w lines to the HiZ
	PORTDIO &= ndallas_low;
	DDRADC &= ndallas_high & 0x7F;
	PORTADC &= ndallas_high & 0x7F;
	if(!(ndallas_high & 0x80))
	{
		cbi(DDRB,7);
		cbi(PORTB,7);
	}
}

