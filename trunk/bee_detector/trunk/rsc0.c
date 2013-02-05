#include <avr/io.h>
#include <inttypes.h>

#include "data_max.h"
#include "rsc0.h"
#include "rsc.h"

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

uint8_t uart0_buffer[32];
volatile uint8_t uart0_head;
volatile uint8_t uart0_tail;
uint8_t SPEED0, BULK0;

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//

//----------------------------------------------------
//Initialisation of the port0

void Ser_com0_Init(uint8_t speed)
{
  uint8_t pom;
  
	cli();
	UBRR0H = 0x00;
	UBRR0L = speed;
	UCSR0A = 0xC0;
	UCSR0B = 0xD8;	//B8; vymenil som utxcie za udrie enable TX, RX and ISRs in UART
	UCSR0C = 0x06;
	pom = UDR0; 		//vycitanim UDR zrusi pripadny int pre rx
	sei();
	stat_TX0 &= 0xC0;
	stat_TX0 |= 0x04;	//							TX.2 = 1
	Ser_com0_Reinit();
}

//---------------------------------------------------
//reinitialisation ot the port0

void Ser_com0_Reinit(void)
{
	uart0_head = 0;
	uart0_tail = 0;
	stat_RX0 = 0x21;		//0x21;set comm thread, RX buff empty, URX
#ifndef DS_3
	if(!PWR)		// Ak PWR = 0; zapni prijem na rs485?
	{
		R485on;		
		T485off;
	}
#endif  //DS_3
}

//---------------------------------------------------
//Services global
//---------------------------------------------------

void Ser_com0(void)
{
 register uint8_t service;
 
	service = Ser_com_rxb(0);

	if(!(stat_RX0 & 0x08))			//if not bulk				!RX.3?
	{
	 	if(!(stat_STAT & 0x01))		//if not measuring			!STAT.0?
	 	{
	   		Bulk_outmeas(service,0);
	  		SC_outmeas(service,0);
	 	}
	 	Bulk_inmeas(service,0);
 	 	SC_inmeas(service,0);
 	}
	else
	{					//else bulk
       		if(!(stat_STAT & 0x01))		//if not measuring			!STAT.0?
       			Bulk_outmeas(service,0);
	 	Bulk_inmeas(service,0);
	}
}

/*--------------------------------------------------------------------------------
	Communication speed test
*/

uint8_t Comratetest0(void)
{
 uint8_t pom,l;
 
	UCSR0B = 0x58;			//RXCIE disable - software control
	my_delay(14,0xC2);		//maximalna doba 350ms?
	stat_TX0 &= 0xEF;		//pretoze dojde k nastaveniu v T2		TX.4 = 0
	for(l = 0;l<10;l++)		//cakam 10 testovacich bytov
	{
		while(bit_is_clear(UCSR0A,7));	//softverove riadenie prijmu -> cakam na prijatie bytu
		{
			if(!i)			//ak medzi tym pretiekol cas, tak vrat neuspesny pokus
				return(0);
		}
		pom = UDR0;			//vycitaj data
		if(pom != 0xAA)			//ak sa nerovnaju test. vzorke 0xAA tak zrus casovac
		{
			i = 1;
			while(i);
		 	return(0);		//vrat neuspesny pokus
		}
	}
	i = 1;			//ak preslo vsetkych 10 bytov, tak zrus casovac
	while(i);
	return(1);		// a vrat uspesny pokus
}
