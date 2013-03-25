#ifndef _ROUTINES_H_
#define _ROUTINES_H_

#include "rtc.h"

//uint8_t readCfgFile(DATE *adate,TIME *atime, uint8_t *filename, uint8_t atomic);
uint8_t readCfgFile(uint8_t *filename, uint8_t atomic);

uint8_t getDec(uint8_t *data);

int8_t search_mask(int8_t previous,uint16_t mask);

void generate_mask_bm(void);

#endif //_ROUTINES_H_
