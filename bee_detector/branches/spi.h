/*! \file spi.h \brief SPI interface driver. */
//*****************************************************************************
//
// File Name	: 'spi.h'
// Title		: SPI interface driver
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 06/06/2002
// Version		: 0.6
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// NOTE: This code is currently below version 1.0, and therefore is considered
// to be lacking in some functionality or documentation, or may not be fully
// tested.  Nonetheless, you can expect most functions to work.
//
///	\ingroup driver_avr
/// \defgroup spi SPI (Serial Peripheral Interface) Function Library (spi.c)
/// \code #include "spi.h" \endcode
/// \par Overview
///		Provides basic byte and word transmitting and receiving via the AVR
///	SPI interface.  Due to the nature of SPI, every SPI communication operation
/// is both a transmit and simultaneous receive.
///
///	\note Currently, only MASTER mode is supported.
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef SPI_H_
#define SPI_H_


//#define SPI_HIGH_SPEED	SPCR &= ~(_BV(SPR1)); SPSR |= (_BV(SPI2X))
//#define SPI_LOW_SPEED		SPCR |= (_BV(SPR1)); SPSR &= ~(_BV(SPI2X))
#define SPI_HIGH_SPEED		SPI0.CTRL|=SPI_CLK2X_bm			//(_BV(CLK2X))
#define SPI_LOW_SPEED		SPI0.CTRL&=~SPI_CLK2X_bm

#define SPI_PRESCALER_4_gc		0
#define SPI_PRESCALER_16_gc		1
#define SPI_PRESCALER_64_gc		2
#define SPI_PRESCALER_128_gc	3
		
#define SPI0_PORT			PORT(SPI0_P)
#define SPI0				SPI(SPI0_P)
#define SPI1_PORT			PORT(SPI1_P)
//#define SPI1_DDR			DDR(SPI1_P)
#define BE					0
#define LE					1

// function prototypes

// SPI interface initializer
void SPI_Init(void);

void SPI_Switch(uint8_t data);

void SPI_Send8(uint8_t data, uint8_t port);

uint8_t SPI_Receive8(uint8_t port);

uint8_t SPI_Transfer8(uint8_t data, uint8_t port);

uint16_t SPI_Transfer16(uint16_t data, uint8_t endian, uint8_t port);

#endif
