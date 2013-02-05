
//-----------------------------------------------------------

void ow_sendbit(register uint8_t dallas,register uint8_t data)
{
 register uint8_t pomx = 0,pom2,pom1,pom20,pom10;
	
 	pom2 = PORTADC;
 	pom20 = pom2 & ~(dallas);
 	PORTADC = pom20;   			//clear 1w lines low byte

	cli();	

//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	DELAY_US(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	if(data & 0x01)				//				!data.0
	{
		PORTADC = pom2;                 //if send 1, set the lines
	}

	delay_us(ONE_WIRE_WRITE_SLOT_TIME_US);
	
	sei();
 	PORTADC = pom2;                         // set the 1W lines
}

//-----------------------------------------------------------

uint8_t ow_recbit(register uint8_t dallas)
{
 register uint8_t pomx = 0,pom4,pom2,pom20,pom40;

 	pom2 = PORTADC;
 	pom20 = pom2 & ~(dallas);
 	pom4 = DDRADC;
	pom40 = pom4 & ~(dallas);
	
	cli();					//disable interrupts (time critical)
	
	PORTADC = pom20;   			//clear 1w lines low byte
	
//	DELAY_OW_RECOVERY_TIME();		//pre 8535
	DELAY_US(ONE_WIRE_RECOVERY_TIME_US);	//pre mega128

	PORTADC = pom2;                         //active pull-up najprv vnutit tvrdu jeddnotku ako vystup
	DDRADC = pom40;                         //a potom prepnut na vstup s pull-up odporom

	
	DELAY_US(13);
	

	pom2 = PINADC;				//high byte

	sei();

	delay_us(ONE_WIRE_READ_SLOT_TIME_US);

 	DDRADC = pom4;                          //return ports to init state for 1w comm (outpu, H on pin)
	return(pom2);
}

//-----------------------------------------------------------

uint8_t ow_reset(uint8_t dallas)
{

 register uint8_t pomx = 0,pom4,pom3,pom2,pom1;
 uint8_t pom20,pom10;

//	dallas_low = (uint8_t)dallas;
//	dallas_high = (uint8_t)(dallas>>8) & 0x7F;
// 	pom10 = PORTDIO;
 	pom20 = PORTADC;
//	pom3 = DDRDIO;
	pom4 = DDRADC;
// 	pom1 = pom10;
 	pom2 = pom20;
//	PORTDIO = pom1 | dallas_low;		//set 1w lines low byte
	PORTADC = pom2 | dallas;		//high byte

	delay_us(250);				//na odstranenie dummy resetu (pred tymto boli draty na 0 takze hned pride pulz a potom sa uz nic nedeje :-( )

//	DDRDIO = pom3 | dallas_low;		//set 1w lines as output low byte
	DDRADC = pom4 | dallas;			//high byte

//	PORTDIO = pom1 & ~dallas_low;			//clear 1w lines low byte
	PORTADC = pom2 & ~dallas;			//high byte

	delay_us(550);

	cli();							//disable interrupts
//	DDRDIO = pom3 & ~dallas_low;			//inputs 1w (low byte)
//	PORTDIO = pom1 | dallas_low;			//set 1w lines low byte (passive pull-up)
	DDRADC = pom4 & ~dallas;			//inputs 1w (high byte)
	PORTADC = pom2 | dallas;			//set 1w lines high byte (passive pull-up)

	delay_us(70);

//	pom1 = PINDIO;				//citaj presence pulse -> 0 znamena pritomny
	pom2 = PINADC;

	delay_us(300);
	
//	pom1 = pom1 | ~PINDIO;			//ak tam skutocne je, 0 musi po case zmiznut.
	pom2 = pom2 | ~(PINADC);			// ak nezmizne, tak sa povodna maska upravi
	
	sei();			//enable interrupts

	delay_us(250);
//	pom1 = ~pom1;
	pom2 = ~pom2;

//	DDRDIO = (pom3 | pom1);		//vrat port do povodneho stavu, ale kde najde 1w ostane vystup!
//	PORTDIO = (pom10 | pom1);		//vrat port do povodneho stavu, ale kde je 1w tam bude H!
	DDRADC = (pom4 | pom2);
	PORTADC = (pom20 | pom2);

	return(pom2);			//inverted result is in zl
}

//-----------------------------------------------------------

void ow_rstprt(register uint8_t dallas)
{
// register uint8_t ndallas_low = ~(uint8_t)dallas;
 register uint8_t ndallas = ~(dallas);
 
//	DDRDIO &= ndallas_low;		//put all 1w lines to the HiZ
//	PORTDIO &= ndallas_low;
	DDRADC &= ndallas;
	PORTADC &= ndallas;
}



