//**************************************************************
// ****** FUNCTIONS FOR SD RAW DATA TRANSFER *******
//**************************************************************
//Controller: ATmega32 (Clock: 8 Mhz-internal)
//Compiler	: AVR-GCC (winAVR with AVRStudio)
//Version 	: 2.4
//Author	: CC Dharmani, Chennai (India)
//			  www.dharmanitech.com
//Date		: 17 May 2010
//**************************************************************

//Link to the Post: http://www.dharmanitech.com/2009/01/sd-card-interfacing-with-atmega8-fat32.html

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>

#include "include/hw.h"
#include "include/spi.h"
#include "include/SD_routines.h"
//#include "UART_routines.h"

#if BIGAVR == 0
//ISR(SD_DETECT_INT)
//{
//	EIMSK &= ~(_BV(INT1));
//	Status |= SD_PRESENT;
//	Status &= ~SD_NOTPRESENT;
//	sei();
//}
#endif		//BIGAVR

ISR(SD_CD_INT)
{
	SD_Status |= SD_PRESENT;
	PORTCFG.MPCMASK = _BV(SD_CD);
	SD_DETECT_PORT.INT0MASK = 0;		// zrus int od card detect
}

ISR(SW0_INT)
{
	SD_Status |= SD_WRITE_BREAK;
	PORTCFG.MPCMASK = _BV(DS0);
	SWITCH_PORT.INT0MASK &= ~(_BV(DS0));		// zrus int od dip switch-u 0

}

//******************************************************************
//Function	: to initialize the SD/SDHC card port 
//Arguments	: none
//return	: none
//******************************************************************
void SD_Init_hw(void)
{
	SD_CS_PORT.DIRSET = _BV(SD_CS);			// release chip select
	SD_CS_PORT.OUTSET = _BV(SD_CS);

	SD_DETECT_PORT.DIRCLR = _BV(SD_CD);
	PORTCFG.MPCMASK = _BV(SD_CD);
	SD_DETECT_PORT.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_LEVEL_gc;		// pull up enable and low level sense

//setting up interrupt
	SD_DETECT_PORT.INT0MASK = _BV(SD_CD);					// INT0 pins setup
	SD_DETECT_PORT.INTCTRL = PMIC_INTLVL_LO_gc;				// INT0 level Low ???
	PMIC.CTRL |= PMIC_LOLVLEN_bm;

//switch settings to be moved elsewhere	
	SWITCH_PORT.DIRCLR = _BV(DS0)|_BV(DS1);
	PORTCFG.MPCMASK = _BV(DS0)|_BV(DS1);
	SWITCH_PORT.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_ISC_LEVEL_gc;
	SWITCH_PORT.INT0MASK = _BV(DS0);						// INT0 pins setup
	SWITCH_PORT.INTCTRL = PMIC_INTLVL_MED_gc;				// INT0 level medium ???
	PMIC.CTRL |= PMIC_MEDLVLEN_bm;

}

//******************************************************************
//Function	: to initialize the SD/SDHC card in SPI mode
//Arguments	: uint8_t power 1 - already on, 2 - init
//return	: uint8_t; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
uint8_t SD_Init(uint8_t power)
{
	uint8_t i, response, SD_version;
	uint16_t retry=0 ;

// after each power up
	if(power != 1)					
	{
//		SD_POWER_ON;
		
		for(i=0;i<10;i++)
			SPI_Send8(0xff,SD_SPI_PORT);   	//80 clock pulses spent before sending the first command

		SD_CS_ASSERT;
		do
		{
			response = SD_sendCommand(GO_IDLE_STATE, 0); //send 'reset & go idle' command
			retry++;
			if(retry>0x20) 
				return(1);   	//time out, card not detected
		} while(response != 0x01);

		SD_CS_DEASSERT;
		SPI_Send8(0xff,SD_SPI_PORT);
		SPI_Send8(0xff,SD_SPI_PORT);

		retry = 0;

		SD_version = 2; 		//default set to SD compliance with ver2.x; 
							//this may change after checking the next command
		do
		{
			response = SD_sendCommand(SEND_IF_COND,0x000001AA); //Check power supply status, mandatory for SDHC card
			retry++;
			if(retry>0xfe) 
			{
				SD_version = 1;
				cardType = 1;
				break;
			} //time out

		}while(response != 0x01);
	}
	
// only if activation from idle
	retry = 0;

	do
	{
		response = SD_sendCommand(APP_CMD,0); //CMD55, must be sent before sending any ACMD command
		response = SD_sendCommand(SD_SEND_OP_COND,0x40000000); //ACMD41
		retry++;
		if(retry>0xfe) 
		{
			return 2;  		//time out, card initialization failed
		} 
	}while(response != 0x00);

//only in init
	if(power == 2)		
	{
		retry = 0;
		SDHC_flag = 0;

		if (SD_version == 2)
		{ 
			do
			{
				response = SD_sendCommand(READ_OCR,0);
				retry++;
				if(retry>0xfe) 
				{
					cardType = 0;
					break;
				} //time out
			}while(response != 0x00);

			if(SDHC_flag == 1) 
				cardType = 2;
			else 
				cardType = 3;
		}
	}	

//SD_sendCommand(CRC_ON_OFF, OFF); //disable CRC; deafault - CRC disabled in SPI mode
//SD_sendCommand(SET_BLOCK_LEN, 512); //set block size to 512; default size is 512

	return(0); //successful return
}

//******************************************************************
//Function	: to put SD card into Idle mode
//Arguments	: uint8_t (8-bit power) 1 - keep power on
//return	: power status  1 - power on
//******************************************************************

/*uint8_t SD_Idle(uint8_t power)
{
	uint8_t response, retry = 0;

	SD_CS_ASSERT;
	do
	{
		response = SD_sendCommand(GO_IDLE_STATE, 0); //send 'reset & go idle' command
		retry++;
		if(retry>0x20) 
			return(1);   	//time out, card not detected
	} while(response != 0x01);

	SD_CS_DEASSERT;
	SPI_Send8(0xff,SD_SPI_PORT);
	SPI_Send8(0xff,SD_SPI_PORT);
	
	if(power)
		return(1);
	SD_POWER_OFF;
	return(0);
}
*/

//******************************************************************
//Function	: to send a command to SD card
//Arguments	: uint8_t (8-bit command value)
// 			  & uint32_t (32-bit command argument)
//return	: uint8_t; response byte
//******************************************************************
uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg)
{
	uint8_t response, retry=0, status;

	//SD card accepts byte address while SDHC accepts block address in multiples of 512
	//so, if it's SD card we need to convert block address into corresponding byte address by 
	//multipying it with 512. which is equivalent to shifting it left 9 times
	//following 'if' loop does that

	if(SDHC_flag == 0)		
		if(cmd == READ_SINGLE_BLOCK     ||
		   cmd == READ_MULTIPLE_BLOCKS  ||
           cmd == WRITE_SINGLE_BLOCK    ||
		   cmd == WRITE_MULTIPLE_BLOCKS ||
		   cmd == ERASE_BLOCK_START_ADDR|| 
		   cmd == ERASE_BLOCK_END_ADDR ) 
		{
			arg = arg << 9;
		}	   

	SD_CS_ASSERT;

	SPI_Send8(cmd | 0x40,SD_SPI_PORT);		//send command, first two bits always '01'
	SPI_Send8(arg>>24,SD_SPI_PORT);
	SPI_Send8(arg>>16,SD_SPI_PORT);
	SPI_Send8(arg>>8,SD_SPI_PORT);
	SPI_Send8(arg,SD_SPI_PORT);

	if(cmd == SEND_IF_COND)	 				//it is compulsory to send correct CRC for CMD8 (CRC=0x87) & CMD0 (CRC=0x95)
		SPI_Send8(0x87,SD_SPI_PORT); 		//for remaining commands, CRC is ignored in SPI mode
	else 
		SPI_Send8(0x95,SD_SPI_PORT); 
	
	while((response = SPI_Receive8(SD_SPI_PORT)) == 0xff) //wait response
		if(retry++ > 0x1e)					// bolo 0xfe, ale pri SEND_IF_COND po power up strasne dlho caka na opakovanie) 
			break; //time out error

	if(response == 0x00 && cmd == 58)  	//checking response of CMD58
	{
		status = SPI_Receive8(SD_SPI_PORT) & 0x40;    //first byte of the OCR register (bit 31:24)
		if(status == 0x40) 
			SDHC_flag = 1;  				//we need it to verify SDHC card
		else 
			SDHC_flag = 0;

		SPI_Receive8(SD_SPI_PORT); 					//remaining 3 bytes of the OCR register are ignored here
		SPI_Receive8(SD_SPI_PORT); 					//one can use these bytes to check power supply limits of SD
		SPI_Receive8(SD_SPI_PORT); 
	}

	SPI_Receive8(SD_SPI_PORT); 						//extra 8 CLK
	SD_CS_DEASSERT;							//aj pri inite?

	return(response); 						//return state
}

//*****************************************************************
//Function	: to erase specified no. of blocks of SD card
//Arguments	: none
//return	: uint8_t; will be 0 if no error,
// 			  otherwise the response byte will be sent
//*****************************************************************
uint8_t SD_erase (uint32_t startBlock, uint32_t totalBlocks)
{
	uint8_t response;

	response = SD_sendCommand(ERASE_BLOCK_START_ADDR, startBlock); 					//send starting block address
	if(response != 0x00) 																//check for SD status: 0x00 - OK (No flags set)
		return(response);

	response = SD_sendCommand(ERASE_BLOCK_END_ADDR,(startBlock + totalBlocks - 1)); 	//send end block address
	if(response != 0x00)
		return(response);

	response = SD_sendCommand(ERASE_SELECTED_BLOCKS, 0); 								//erase all selected blocks
	if(response != 0x00)
		return(response);

	return(0); 																		//normal return
}

//******************************************************************
//Function	: to read a single block from SD card
//Arguments	: none
//return	: uint8_t; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
uint8_t SD_readSingleBlock(uint32_t startBlock, uint8_t atomic)
{
	uint8_t response;
	uint16_t i, retry=0;

	if(atomic)
	{
		cli();
	}

	response = SD_sendCommand(READ_SINGLE_BLOCK, startBlock); //read a Block command
 
	if(response != 0x00) 
		return(response); 			//check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	retry = 0;
	while(SPI_Receive8(SD_SPI_PORT) != 0xfe) 	//wait for start block token 0xfe (0x11111110)
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		} 							//return if time-out

	for(i=0; i<512; i++) 			//read 512 bytes
		buffer[i] = SPI_Receive8(SD_SPI_PORT);

	SPI_Receive8(SD_SPI_PORT);		//receive incoming CRC (16-bit), CRC is ignored here
	SPI_Receive8(SD_SPI_PORT);

	SPI_Receive8(SD_SPI_PORT);		//extra 8 clock pulses
	SD_CS_DEASSERT;

	if(atomic)
	{
		sei();
	}

	return(0);
}

//******************************************************************
//Function	: to write to a single block of SD card
//Arguments	: none
//return	: uint8_t; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
uint8_t SD_writeSingleBlock(uint32_t startBlock, uint8_t atomic)
{
	uint8_t response;
	uint16_t i, retry=0;

	if(atomic)
	{
		cli();
	}

	response = SD_sendCommand(WRITE_SINGLE_BLOCK, startBlock); //write a Block command
  
	if(response != 0x00) 
		return(response); 				//check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	SPI_Send8(0xfe,SD_SPI_PORT);   		//Send start block token 0xfe (0x11111110)
		
	for(i=0; i<512; i++)    			//send 512 bytes data
		SPI_Send8(buffer[i],SD_SPI_PORT);

	SPI_Send8(0xff,SD_SPI_PORT);   		//transmit dummy CRC (16-bit), CRC is ignored here
	SPI_Send8(0xff,SD_SPI_PORT);

	response = SPI_Receive8(SD_SPI_PORT);

	if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
	{                              	//AAA='101'-data rejected due to CRC error
		SD_CS_DEASSERT;              //AAA='110'-data rejected due to write error
		return(response);
	}

	while(!SPI_Receive8(SD_SPI_PORT)) //wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		}

	SD_CS_DEASSERT;
	SPI_Send8(0xff,SD_SPI_PORT);   //just spend 8 clock cycle delay before reasserting the CS line
	SD_CS_ASSERT;         //re-asserting the CS line to verify if card is still busy

	while(!SPI_Receive8(SD_SPI_PORT)) //wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		}

	SD_CS_DEASSERT;

	if(atomic)
	{
		sei();
	}

	return(0);
}


#ifndef FAT_TESTING_ONLY

//***************************************************************************
//Function	: to read multiple blocks from SD card & send every block to UART
//Arguments	: none
//return	: uint8_t; will be 0 if no error,
// 			  otherwise the response byte will be sent
//****************************************************************************
uint8_t SD_readMultipleBlock (uint32_t startBlock, uint32_t totalBlocks)
{
	uint8_t response;
	uint16_t i, retry=0;



	response = SD_sendCommand(READ_MULTIPLE_BLOCKS, startBlock); //write a Block command
  
	if(response != 0x00) 
		return(response); 				//check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;

	while( totalBlocks )
	{
		retry = 0;
		while(SPI_Receive8(SD_SPI_PORT) != 0xfe) 	//wait for start block token 0xfe (0x11111110)
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		} 								//return if time-out

		for(i=0; i<512; i++) 			//read 512 bytes
			buffer[i] = SPI_Receive8(SD_SPI_PORT);

		SPI_Receive8(SD_SPI_PORT); 				//receive incoming CRC (16-bit), CRC is ignored here
		SPI_Receive8(SD_SPI_PORT);

		SPI_Receive8(SD_SPI_PORT); 				//extra 8 cycles
		printf_P(PSTR("\n --------- \n"));

		for(i=0; i<512; i++) 			//send the block to UART
		{
			if(buffer[i] == '~') 
				break;
			printf("%c", buffer[i] );
		}

		printf_P(PSTR("\n --------- \n"));
		totalBlocks--;
	}

	SD_sendCommand(STOP_TRANSMISSION, 0); //command to stop transmission
	SD_CS_DEASSERT;
	SPI_Receive8(SD_SPI_PORT); 					//extra 8 clock pulses

	return(0);
}

//***************************************************************************
//Function: to receive data from UART and write to multiple blocks of SD card
//Arguments: none
//return: uint8_t; will be 0 if no error,
// otherwise the response byte will be sent
//****************************************************************************
uint8_t SD_writeMultipleBlock(uint32_t startBlock, uint32_t totalBlocks)
{
	uint8_t response, data = 0;
	uint16_t i, retry=0;
	uint32_t blockCounter=0;

	response = SD_sendCommand(WRITE_MULTIPLE_BLOCKS, startBlock); 	//write a Block command

	if(response != 0x00) 
		return(response); 											//check for SD status: 0x00 - OK (No flags set)

	SD_CS_ASSERT;
	printf_P(PSTR("\n Enter text (End with ~): \n"));

	while( blockCounter < totalBlocks )
	{
		i=0;
//		do
//		{
//			data = receiveByte();
//			if(data == 0x08)											//'Back Space' key pressed
//			{ 
//				if(i != 0)
//				{ 
//					transmitByte(data);
//					transmitByte(' '); 
//					transmitByte(data); 
//					i--; 
//					size--;
//				} 
//				continue;     
//			}
//			transmitByte(data);
			buffer[i++] = data;
//			if(data == 0x0d)
//			{
//				transmitByte(0x0a);
//				buffer[i++] = 0x0a;
//			}
//			if(i == 512) break;
//		}while (data != '~');

		printf_P(PSTR("\n ---- \n"));
		
		SPI_Send8(0xfc,SD_SPI_PORT); 				//Send start block token 0xfc (0x11111100)

		for(i=0; i<512; i++) 			//send 512 bytes data
			SPI_Send8( buffer[i],SD_SPI_PORT);

		SPI_Send8(0xff,SD_SPI_PORT); 				//transmit dummy CRC (16-bit), CRC is ignored here
		SPI_Send8(0xff,SD_SPI_PORT);

		response = SPI_Receive8(SD_SPI_PORT);
		if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
		{                              	//AAA='101'-data rejected due to CRC error
			SD_CS_DEASSERT;             //AAA='110'-data rejected due to write error
			return(response);
		}

		while(!SPI_Receive8(SD_SPI_PORT)) 			//wait for SD card to complete writing and get idle
			if(retry++ > 0xfffe)
			{
				SD_CS_DEASSERT; 
				return(1);
			}

		SPI_Receive8(SD_SPI_PORT); 				//extra 8 bits
		blockCounter++;
	}

	SPI_Send8(0xfd,SD_SPI_PORT); 					//send 'stop transmission token'

	retry = 0;

	while(!SPI_Receive8(SD_SPI_PORT)) 				//wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		}

	SD_CS_DEASSERT;
	SPI_Send8(0xff,SD_SPI_PORT); 					//just spend 8 clock cycle delay before reasserting the CS signal
	SD_CS_ASSERT; 						//re assertion of the CS signal is required to verify if card is still busy

	while(!SPI_Receive8(SD_SPI_PORT)) 				//wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT; 
			return(1);
		}
	SD_CS_DEASSERT;

	return(0);
}
//*********************************************

#endif

//******** END ****** www.dharmanitech.com *****
