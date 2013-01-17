/*! \file xspi.c \brief SPI interface driver for xmega. */
//*****************************************************************************
//
// File Name	: 'xspi.c'
// Title		: SPI interface driver
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 06/06/2002
// Version		: 0.6
// Target MCU	: Atmel AVR xmega series
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "include/hw.h"
#include "include/spi.h"

// Define the SPI_USEINT key if you want SPI bus operation to be
// interrupt-driven.  The primary reason for not using SPI in
// interrupt-driven mode is if the SPI send/transfer commands
// will be used from within some other interrupt service routine
// or if interrupts might be globally turned off due to of other
// aspects of your program
//
// Comment-out or uncomment this line as necessary
//#define SPI_USEINT

// global variables

#ifdef SPI_USEINT
volatile uint8_t spiTransferComplete;
#endif


// SPI interrupt service handler
#ifdef SPI_USEINT
//test
ISR(SPI0_int)
{
	spiTransferComplete = TRUE;
}

//ISR(USART_RX_vect)
//{
//	spi1TransferComplete = TRUE;
//}

#endif

// access routines

void SPI_Init(void)
{
	uint8_t temp;
	
	SPI0_PORT.DIRSET = _BV(SPI0_SCK)|_BV(SPI0_MOSI)|_BV(SPI0_SS);	//set outputs
//	SPI0.DIRCLR = _BV(SPI0_MISO);								//set input
	SPI0_PORT.OUT |= _BV(SPI0_SS);									//set ss Hi 
	
/* setup SPI interface :
	SPR0 = 1	
	SPR1 = 0	// fclk/16
	CPHA = 1?	// rising edge sample, falling edge setup  	was 0 	for LIS3LV 1???
	CPOL = 1?	// sck is HI when idle						was 0	for LIS3LV 1???
	MSTR = 1	// master
	DORD = 0	// MSB first
	SPE  = 1	// SPI enable		*/

	SPI0.CTRL = SPI_MASTER_bm | SPI_ENABLE_bm | SPI_MODE_0_gc | SPI_PRESCALER_64_gc;	
	
	temp = SPI0.STATUS;						// clear status
	temp = SPI0.DATA;


	#ifdef SPI_USEINT
//		spi1TransferComplete = TRUE;			// enable USART SPI RX interrupt
//		UCSR0B |= _BV(RXCIE0);
	#endif

	#ifdef SPI_USEINT
		spiTransferComplete = TRUE;				// enable SPI interrupt
		SPI0.INTCTRL = PMIC_INTLEVEL_MED_gc;	//0x02;					// medium level interrupt
	#endif
}


void SPI_Send8(uint8_t data, uint8_t port)
{
	uint8_t dump;
//	if(port)
//	{
//		UDR0 = data;
//		#ifdef SPI_USEINT
//			while(!spi1TransferComplete);	// send a byte over SPI and ignore reply
//			spi1TransferComplete = FALSE;
//		#else
//			while(!(UCSR0A & _BV(RXC0)));
//		#endif
//		dump = UDR0;
//	}
//	else
//	{
		SPI0.DATA = data;
		#ifdef SPI_USEINT
			while(!spiTransferComplete);	// send a byte over SPI and ignore reply
			spiTransferComplete = FALSE;
		#else
			while(!(SPI0.STATUS & SPI_IF_bm));
		#endif
		dump = SPI0.DATA;
//	}
}


uint8_t SPI_Receive8(uint8_t port)
{
//	if(port)
//	{
//		UDR0 = 0xff;
//	
//		#ifdef SPI_USEINT
//			while(!spiTransferComplete);	// send a byte over SPI and ignore reply
//		#else
//			while(!(UCSR0A & _BV(RXC0)));
//		#endif
//		return(UDR0);
//	}
//	else
//	{
		SPI0.DATA = 0xff;

		#ifdef SPI_USEINT
			while(!spiTransferComplete);	// send a byte over SPI and ignore reply
		#else
			while(!(SPI0.STATUS & SPI_IF_bm));
		#endif
		return(SPI0.DATA);
//	}
}


uint8_t SPI_Transfer8(uint8_t data, uint8_t port)
{
//	if(port)
//	{
//		#ifdef SPI_USEINT
//			spi1TransferComplete = FALSE;		// send the given data
//			UDR0 = data;
//			while(!spi1TransferComplete);		// wait for transfer to complete
//		#else
//			UDR0 = data;						// send the given data
//			while(!(UCSR0A & _BV(RXC0)));		// wait for transfer to complete
//		#endif
//		return(UDR0);							// return the received data
//	}
//	else
//	{
		#ifdef SPI_USEINT
			spiTransferComplete = FALSE;		// send the given data
			SPI0.DATA = data;
			while(!spiTransferComplete);		// wait for transfer to complete
		#else
			SPI0.DATA = data;						// send the given data
			while(!(SPI0.STATUS & SPI_IF_bm));		// wait for transfer to complete
		#endif
		return(SPI0.DATA);						// return the received data
//	}
}


uint16_t SPI_Transfer16(uint16_t data, uint8_t endian, uint8_t port)
{
	uint16_t rxData = 0;
	if(endian == BE)
	{
		rxData = (SPI_Transfer8((uint8_t)(data>>8), port))<<8;	// send MS byte of given data
		rxData |= (SPI_Transfer8((uint8_t)data, port));			// send LS byte of given data
	}
	else
	{
		rxData = SPI_Transfer8((uint8_t)data, port);				// send LS byte of given data
		rxData |= (SPI_Transfer8((uint8_t)(data>>8), port))<<8;	// send MS byte of given data
	}

	return(rxData);										// return the received data
}

