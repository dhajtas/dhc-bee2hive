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

#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_

#include "hw.h"


//Use following macro if you don't want to activate the multiple block access functions
//those functions are not required for FAT32

//#define FAT_TESTING_ONLY

#define SD_CS_PORT		PORT(SD_CTRL_PORT)
#define SD_DETECT_PORT	PORT(SD_CTRL_PORT)

#define SD_CS_ASSERT    SD_CS_PORT.OUTCLR=_BV(SD_CS)
#define SD_CS_DEASSERT  SD_CS_PORT.OUTSET=_BV(SD_CS)
#define SD_POWER_ON
#define SD_POWER_OFF


//SD commands, many of these are not used here
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND			 8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND			 41   //ACMD
#define APP_CMD					 55
#define READ_OCR				 58
#define CRC_ON_OFF               59


#define ON     1
#define OFF    0

volatile uint32_t startBlock, totalBlocks; 
volatile uint8_t SDHC_flag, cardType, buffer[512];

void SD_Init_hw(void);

uint8_t SD_Init(uint8_t power);

uint8_t SD_Idle(uint8_t power);

uint8_t SD_sendCommand(uint8_t cmd, uint32_t arg);

uint8_t SD_erase (uint32_t startBlock, uint32_t totalBlocks);

uint8_t SD_readSingleBlock(uint32_t startBlock, uint8_t atomic);

uint8_t SD_writeSingleBlock(uint32_t startBlock, uint8_t atomic);

#ifndef FAT_TESTING_ONLY
	uint8_t SD_readMultipleBlock (uint32_t startBlock, uint32_t totalBlocks);
	
	uint8_t SD_writeMultipleBlock(uint32_t startBlock, uint32_t totalBlocks);
#endif

#endif
