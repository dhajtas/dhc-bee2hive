#ifndef	OWIRE_H
#define OWIRE_H

//-----------------------------------------------------------------------------------------------//
//		structure definitions 
//-----------------------------------------------------------------------------------------------//

#define		OW_PORT		PORT(I2C_P)

typedef struct OWIRE
{
	uint8_t data[8];
} OWIRE_t;

//-----------------------------------------------------------------------------------------------//
//		Global variables
//-----------------------------------------------------------------------------------------------//

extern OWIRE_t OWbuff[OW_NUM];

//-----------------------------------------------------------------------------------------------//
//		routines
//-----------------------------------------------------------------------------------------------//


uint8_t ow_Init(void);

void clr_owBuffer(void);

uint8_t GetDallasID(uint8_t);

void owire(uint8_t, uint8_t);	// OWIRE*, uint8_t, uint8_t);

uint8_t ow_reset(register uint8_t dallas);

void ow_rstprt(register uint8_t dallas);

void ow_setprt(register uint8_t dallas);

void ow_outp(register uint8_t data);

uint8_t ow_inp(register uint8_t dallas);

void ow_sendbit(register uint8_t data);

uint8_t ow_recbit(register uint8_t dallas);

uint8_t CheckCRC_8(OWIRE_t*, uint8_t, uint8_t);	//uint8_t,

uint8_t CRC_8(uint8_t, uint8_t);

uint16_t CRC_16(uint16_t, uint8_t);

#ifndef  F_CPU
#define  F_CPU		              32000000   /* The cpu clock frequency in Hertz */
#endif

#define ONE_WIRE_RECOVERY_TIME_US     2       /* The time for the pullup resistor to get the line high */
#define ONE_WIRE_RESET_TIME_US        1000    /* The total reset pulse time */
#define ONE_WIRE_PRESENSE_TIME_US     70     /* The time of the presence pulse detection */
#define ONE_WIRE_WRITE_SLOT_TIME_US   90      /* The total time of the WRITE SLOT  */ 
#define ONE_WIRE_READ_SLOT_TIME_US    60      /* The total time of the READ SLOT  */ 

#endif //OWIRE_H
