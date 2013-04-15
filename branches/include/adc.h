#ifndef __ADC_H__
#define __ADC_H__

//#include "ffft.h"

//----------------------------------------------------------
//						DEFS
//----------------------------------------------------------

#define ADC3_FINISHED		0x08
#define ADC2_FINISHED		0x04
#define ADC1_FINISHED		0x02
#define ADC0_FINISHED		0x01

#define ADC0_PORT	PORT(ADC0_P)
#define ADC1_PORT	PORT(ADC1_P)

#define ADC_IMPMODE_LOWIMP_gc 		0x80
#define ADC_IMPMODE_HIIMP_gc 		0x00
#define ADC_CURRLIMIT_NO_gc			0<<5
#define ADC_CURRLIMIT_LO_gc			1<<5
#define ADC_CURRLIMIT_MED_gc		2<<5
#define ADC_CURRLIMIT_HIGH_gc		3<<5
#define ADC_CONVMODE_UNSIGNED_gc	0<<4
#define ADC_CONVMODE_SIGNED_gc		1<<4
#define ADC_FREERUN_OFF_gc			0<<3
#define ADC_FREERUN_ON_gc			1<<3

#if ADC_INT_ENABLE == 1
//extern volatile int16_t ADC_Buffer[];
extern volatile uint8_t ADC_Index;
extern volatile uint8_t ADC_Status;
extern volatile uint16_t ADC_Cal[];
//extern volatile uint8_t ADC_CH0_PIN;
#endif


void ADC_Init(void);

uint8_t ADC_chswitch(uint8_t channel0, uint8_t channel1);

uint16_t ADC_cal(uint8_t channel);

#if ADC_INT_ENABLE == 0
#if ADC_CAPTURE_INPLACE == 1
void ADC_Capture (uint8_t channel, complex_t *bfly_buffer);
#else
void ADC_Capture (uint8_t channel, uint16_t *voltage);
#endif
#endif


#endif //#ifndef __ADC_H__